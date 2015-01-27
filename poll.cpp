#include "poll.h"
#include "messageprocessor.h"
#include "database.h"
#include "namedatabase.h"
#include "subscribe.h"
#include "permission.h"
#include <QtCore>
#include <QSqlQuery>

const int Poll::MaxPollsPerGroup = 10;
const int Poll::MaxOptionsPerPoll = 20;

Poll::Poll(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, Subscribe *sub, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), permission(perm), subscribe(sub)
{
    loadData();
}

Poll::~Poll()
{
    saveData();
}

void Poll::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT gid, id, title, multi_choice, options FROM tf_polls");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        qint64 id = query.value(1).toLongLong();
        QString title = query.value(2).toString();
        bool multi = query.value(3).toBool();

        QByteArray optionsData = QByteArray::fromBase64(query.value(4).toByteArray());
        QDataStream DS(optionsData);
        PollData::Options options;
        DS >> options;

        data[gid][id] = PollData(title, multi, options);
    }
}

void Poll::saveData()
{
    QSqlDatabase::database().transaction();

    QMapIterator<qint64, Polls> groupsIter(data);
    while (groupsIter.hasNext())
    {
        groupsIter.next();
        QMapIterator<qint64, PollData> pollsIter(groupsIter.value());
        while (pollsIter.hasNext())
        {
            pollsIter.next();
            qint64 pollId = pollsIter.key();
            PollData pData = pollsIter.value();

            QByteArray optionsData;
            QDataStream DS(&optionsData, QIODevice::Append);
            DS << pData.options();

            QSqlQuery query;
            query.prepare("UPDATE tf_polls SET options=:options WHERE id=:pid");
            query.bindValue(":options", optionsData.toBase64());
            query.bindValue(":pid", pollId);
            database->executeQuery(query);
        }
    }

    QSqlDatabase::database().commit();
}

void Poll::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!poll"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;
        int perm = 0;

        if (args.size() > 1)
        {
            if (args[1].toLower().startsWith("create") && args.size() > 2)
            {
                bool multi = args[1].toLower().startsWith("create_mul");

                perm = permission->getPermission(gidnum, "poll", multi ? "create_multi" : "create", isAdmin, inpm);
                if (perm == 1)
                {
                    qint64 id = -1;

                    QString title;
                    if (data[gidnum].size() < MaxPollsPerGroup)
                    {
                        int idx = str.indexOf(' ', str.indexOf("create", 0, Qt::CaseInsensitive));
                        while (idx < str.size() && str[idx] == ' ')
                            ++idx;

                        title = str.mid(idx).trimmed();
                        id = createPoll(gidnum, title, multi);
                    }

                    if (id == -1)
                        message = QString("Couldn't add new poll; Maybe delete some polls first.");
                    else
                    {
                        message = QString("Added new poll with id: %1").arg(id);

                        QString subscribeMessage = QString("New Poll: Poll#%1- %2").arg(id).arg(title);
                        subscribe->postToSubscribed(gidnum, subscribeMessage);
                    }
                }
            }
            else if (args[1].toLower().startsWith("title") && args.size() > 3)
            {
                perm = permission->getPermission(gidnum, "poll", "title_change", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok;
                    qint64 id = args[2].toLongLong(&ok);

                    if (ok && data[gidnum].contains(id))
                    {
                        int idx = str.indexOf(' ', str.indexOf(args[2]));
                        while (idx < str.size() && str[idx] == ' ')
                            ++idx;

                        changeTitle(gidnum, id, str.mid(idx).trimmed());
                        message = QString("Poll#%1 title successfully changed.").arg(id);
                    }
                }
            }
            else if (args[1].toLower().startsWith("add") && args.size() > 3)
            {
                perm = permission->getPermission(gidnum, "poll", "add_option", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok;
                    qint64 id = args[2].toLongLong(&ok);

                    if (ok && data[gidnum].contains(id))
                    {
                        if (data[gidnum][id].options().size() < MaxOptionsPerPoll)
                        {
                            int idx = str.indexOf(' ', str.indexOf(args[2]));
                            while (idx < str.size() && str[idx] == ' ')
                                ++idx;

                            QString option = str.mid(idx).trimmed();
                            addOption(gidnum, id, option);
                            message = QString("Added option %1 to poll#%2.")
                                    .arg(data[gidnum][id].options().size()).arg(id);
                        }
                        else
                            message = QString("Couldn't add new option; Maybe delete some options first.");
                    }
                }
            }
            else if ((args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem"))
                     && args.size() > 3)
            {
                perm = permission->getPermission(gidnum, "poll", "delete_option", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok;
                    qint64 id = args[2].toLongLong(&ok);

                    bool ok1;
                    int idx = args[3].toInt(&ok1);

                    if (ok && ok1 && data[gidnum].contains(id) && 0 < idx && idx <= data[gidnum][id].options().size())
                    {
                        delOption(gidnum, id, idx);
                        message = QString("Deleted option %1 From poll#%2.")
                                .arg(idx).arg(id);
                    }
                }
            }
            else if (args[1].toLower().startsWith("option") && args.size() > 4)
            {
                perm = permission->getPermission(gidnum, "poll", "option_change", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok1, ok2;
                    qint64 id = args[2].toLongLong(&ok1);
                    int optid = args[3].toInt(&ok2);

                    if (ok1 && ok2 && data[gidnum].contains(id) && optid <= data[gidnum][id].options().size())
                    {
                        int idx = str.indexOf(' ', str.indexOf(args[2]));
                        ++idx;
                        while (idx < str.size() && str[idx] != ' ')
                            ++idx;
                        while (idx < str.size() && str[idx] == ' ')
                            ++idx;

                        data[gidnum][id].options()[optid - 1].first = str.mid(idx).trimmed();
                        message = QString("Successfully changed option %1 of Poll#%2.").arg(optid).arg(id);
                    }
                }
            }
            else if (args[1].toLower().startsWith("term") && args.size() > 2)
            {
                perm = permission->getPermission(gidnum, "poll", "terminate", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok;
                    qint64 id = args[2].toLongLong(&ok);

                    if (ok && data[gidnum].contains(id))
                    {
                        terminatePoll(gidnum, id);
                        message = QString("Poll#%1 terminated.").arg(id);
                    }
                }
            }
            else if ((args[1].toLower().startsWith("show") || args[1].toLower().startsWith("res"))
                     && args.size() > 2)
            {
                bool result = args[1].toLower().startsWith("res");

                inpm = inpm || (args.size() > 3 && args[3].toLower().startsWith("pm"));

                perm = permission->getPermission(gidnum, "poll", result ? "result" : "show", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok;
                    qint64 id = args[2].toLongLong(&ok);

                    if (ok && data[gidnum].contains(id))
                    {
                        int totalVotes = 0;

                        if (result)
                            foreach (PollData::Option opt, data[gidnum][id].options())
                                totalVotes += opt.second.size();

                        message = QString("Poll#%1- %2%3\n").arg(id).arg(data[gidnum][id].title())
                                .arg(data[gidnum][id].multiChoice() ? " (Multi-Choice)" : "");

                        if (data[gidnum][id].options().isEmpty())
                            message += "No options!";
                        else
                        {
                            int idx = 1;
                            foreach (PollData::Option opt, data[gidnum][id].options())
                            {
                                QString percentStr, resultStr;
                                if (result && !data[gidnum][id].multiChoice() && totalVotes > 0)
                                {
                                    qreal percent = static_cast<qreal>(opt.second.size()) / totalVotes * 100;
                                    percentStr = QString(" - %1%").arg(QString::number(percent, 'f', 2));
                                }

                                if (result)
                                    resultStr = QString(" (%1%2)").arg(opt.second.size()).arg(percentStr);

                                message += QString("%1- %2%3").arg(idx).arg(opt.first).arg(resultStr);

                                if (idx != data[gidnum][id].options().size())
                                    message += "\n";

                                ++idx;
                            }
                        }

                        message += QString("\nUse \"!poll vote %1 your_option\" to vote!").arg(id);
                    }
                }
            }
            else if (args[1].toLower().startsWith("vote") && args.size() > 3)
            {
                perm = permission->getPermission(gidnum, "poll", "vote", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok1, ok2;
                    qint64 id = args[2].toLongLong(&ok1);
                    int option = args[3].toInt(&ok2);

                    if (ok1 && ok2 && data[gidnum].contains(id) && option > 0
                            && option <= data[gidnum][id].options().size())
                    {
                        if (!data[gidnum][id].options()[option - 1].second.contains(uidnum))
                        {
                            bool multi = data[gidnum][id].multiChoice();
                            bool change = false;

                            if (!multi)
                                for (int i = 0; i < data[gidnum][id].options().size(); ++i)
                                    if (data[gidnum][id].options()[i].second.contains(uidnum))
                                    {
                                        data[gidnum][id].options()[i].second.remove(uidnum);
                                        change = true;
                                    }

                            data[gidnum][id].options()[option - 1].second.insert(uidnum);
                            message = QString("%1 %2 in poll#%3 (\"%4\")").arg(messageProcessor->convertToName(uidnum))
                                    .arg(change ? "changed vote" : "voted").arg(id)
                                    .arg(data[gidnum][id].options()[option - 1].first);
                        }
                        else
                            message = QString("%1 has already voted for this option.")
                                    .arg(messageProcessor->convertToName(uidnum));
                    }
                }
            }
            else if (args[1].toLower().startsWith("unvote") && args.size() > 2)
            {
                perm = permission->getPermission(gidnum, "poll", "unvote", isAdmin, inpm);
                if (perm == 1)
                {
                    bool ok1, ok2 = false;
                    qint64 id = args[2].toLongLong(&ok1);

                    int option = -1;
                    if (args.size() > 3)
                        option = args[3].toInt(&ok2);

                    if (ok1 && data[gidnum].contains(id))
                    {
                        bool multi = data[gidnum][id].multiChoice();

                        if (!multi || (ok2 && option > 0 && option <= data[gidnum][id].options().size()))
                        {
                            int unvoted = -1;

                            if (!multi)
                            {
                                for (int i = 0; i < data[gidnum][id].options().size(); ++i)
                                    if (data[gidnum][id].options()[i].second.contains(uidnum))
                                    {
                                        data[gidnum][id].options()[i].second.remove(uidnum);
                                        unvoted = i;
                                        break;
                                    }
                            }
                            else
                            {
                                if (data[gidnum][id].options()[option - 1].second.contains(uidnum))
                                {
                                    unvoted = option - 1;
                                    data[gidnum][id].options()[option - 1].second.remove(uidnum);
                                }
                            }

                            if (unvoted != -1)
                                message = QString("%1 unvoted in poll#%2 (\"%3\")")
                                        .arg(messageProcessor->convertToName(uidnum))
                                        .arg(id).arg(data[gidnum][id].options()[unvoted].first);
                            else
                                message = QString("%1 hasn't voted for this option.")
                                        .arg(messageProcessor->convertToName(uidnum));
                        }
                    }
                }
            }
            else if (args[1].toLower().startsWith("who") && args.size() > 3)
            {
                perm = permission->getPermission(gidnum, "poll", "who", isAdmin, inpm);
                if (perm == 1)
                {
                    inpm = inpm || (args.size() > 4 && args[4].toLower().startsWith("pm"));

                    bool ok1, ok2;
                    qint64 id = args[2].toLongLong(&ok1);
                    int option = args[3].toInt(&ok2);

                    if (ok1 && ok2 && data[gidnum].contains(id) && option > 0
                            && option <= data[gidnum][id].options().size())
                    {
                        message = QString("People voted for \"%1\" in poll#%2:\\n")
                                .arg(data[gidnum][id].options()[option - 1].first)
                                .arg(id);

                        if (data[gidnum][id].options()[option - 1].second.isEmpty())
                            message += "Noone!";
                        else
                        {
                            int count = 0;
                            foreach (qint64 user, data[gidnum][id].options()[option - 1].second)
                            {
                                message += messageProcessor->convertToName(user);
                                if (++count != data[gidnum][id].options()[option - 1].second.size())
                                    message += '-';
                            }
                        }
                    }
                }
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));
            perm = permission->getPermission(gidnum, "poll", "view_list", isAdmin, inpm);
            if (perm == 1)
            {
                QMapIterator<qint64, PollData> dataIter(data[gidnum]);

                if (dataIter.hasNext())
                    while (dataIter.hasNext())
                    {
                        dataIter.next();
                        message += QString("Poll#%1- %2%3").arg(dataIter.key()).arg(dataIter.value().title())
                                .arg(dataIter.value().multiChoice() ? " (Multi-Choice)" : "");
                        if (dataIter.hasNext())
                            message += "\n";
                    }
                else
                    message = QString("No Poll!");
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}

qint64 Poll::createPoll(qint64 gid, const QString &title, bool multi)
{
    QSqlQuery query;
    query.prepare("INSERT INTO tf_polls (gid, title, multi_choice) "
                  "VALUES(:gid, :title, :multi)");
    query.bindValue(":gid", gid);
    query.bindValue(":title", title);
    query.bindValue(":multi", static_cast<int>(multi));
    database->executeQuery(query);

    if (query.lastInsertId().isValid())
    {
        qint64 pid = query.lastInsertId().toLongLong();
        data[gid][pid] = PollData(title, multi);
        return pid;
    }

    return -1;
}

void Poll::addOption(qint64 gid, qint64 id, const QString &opt)
{
    data[gid][id].options().append(PollData::Option(opt, PollData::Users()));
}

void Poll::delOption(qint64 gid, qint64 id, int idx)
{
    data[gid][id].options().removeAt(idx - 1);
}

void Poll::terminatePoll(qint64 gid, qint64 id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_polls WHERE id=:id");
    query.bindValue(":id", id);
    database->executeQuery(query);
    data[gid].remove(id);
}

void Poll::changeTitle(qint64 gid, qint64 pid, const QString &str)
{
    QSqlQuery query;
    query.prepare("UPDATE tf_polls SET title=:title WHERE id=:id");
    query.bindValue(":title", str);
    query.bindValue(":id", pid);
    database->executeQuery(query);
    data[gid][pid].title() = str;
}
