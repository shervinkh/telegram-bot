#include "poll.h"
#include "messageprocessor.h"
#include "database.h"
#include "namedatabase.h"
#include "subscribe.h"
#include <QtCore>
#include <QSqlQuery>

const int Poll::MaxPollsPerGroup = 10;
const int Poll::MaxOptionsPerPoll = 20;

Poll::Poll(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Subscribe *sub, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), subscribe(sub)
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

void Poll::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 usernum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!poll"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 1)
        {
            if (args[1].toLower().startsWith("create") && args.size() > 2)
            {
                qint64 id = -1;

                QString title;
                if (data[gidnum].size() < MaxPollsPerGroup)
                {
                    bool multi = args[1].toLower().startsWith("create_mul");

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
            else if (args[1].toLower().startsWith("add") && args.size() > 3)
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
            else if ((args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem"))
                     && args.size() > 3)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                int start, end;
                int idx = args[3].indexOf('-');
                bool ok1, ok2;

                if (idx == -1)
                {
                    start = end = args[3].toInt(&ok1);
                    ok2 = true;
                }
                else
                {
                    start = args[3].mid(0, idx).toInt(&ok1);
                    end = args[3].mid(idx + 1).toInt(&ok2);
                }


                if (ok && ok1 && ok2 && data[gidnum].contains(id) && 0 < start && end >= start
                    && end <= data[gidnum][id].options().size())
                {
                    for (int i = start; i <= end; ++i)
                        delOption(gidnum, id, start);
                    message = QString("Deleted option %1 From poll#%2.")
                              .arg(args[3]).arg(id);
                }
            }
            else if (args[1].toLower().startsWith("term") && args.size() > 2)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                {
                    terminatePoll(gidnum, id);
                    message = QString("Poll#%1 terminated.").arg(id);
                }
            }
            else if (args[1].toLower().startsWith("list"))
            {
                inpm = inpm || (args.size() > 2 && args[2].toLower().startsWith("pm"));

                QMapIterator<qint64, PollData> dataIter(data[gidnum]);

                if (dataIter.hasNext())
                    while (dataIter.hasNext())
                    {
                        dataIter.next();
                        message += QString("Poll#%1- %2%3").arg(dataIter.key()).arg(dataIter.value().title())
                                   .arg(dataIter.value().multiChoice() ? " (Multi-Choice)" : "");
                        if (dataIter.hasNext())
                            message += "\\n";
                    }
                else
                    message = QString("No Poll!");
            }
            else if ((args[1].toLower().startsWith("show") || args[1].toLower().startsWith("res"))
                     && args.size() > 2)
            {
                bool result = args[1].toLower().startsWith("res");

                inpm = inpm || (args.size() > 3 && args[3].toLower().startsWith("pm"));

                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                {
                    int totalVotes = 0;

                    if (result)
                        foreach (PollData::Option opt, data[gidnum][id].options())
                            totalVotes += opt.second.size();

                    message = QString("Poll#%1- %2%3\\n").arg(id).arg(data[gidnum][id].title())
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
                                message += "\\n";

                            ++idx;
                        }
                    }

                    message += QString("\\nUse \"!poll vote %1 your_option\" to vote!").arg(id);
                }
            }
            else if (args[1].toLower().startsWith("vote") && args.size() > 3)
            {
                bool ok1, ok2;
                qint64 id = args[2].toLongLong(&ok1);
                int option = args[3].toInt(&ok2);

                if (ok1 && ok2 && data[gidnum].contains(id) && option > 0
                    && option <= data[gidnum][id].options().size())
                {
                    if (!data[gidnum][id].options()[option - 1].second.contains(usernum))
                    {
                        bool multi = data[gidnum][id].multiChoice();
                        bool change = false;

                        if (!multi)
                            for (int i = 0; i < data[gidnum][id].options().size(); ++i)
                                if (data[gidnum][id].options()[i].second.contains(usernum))
                                {
                                    data[gidnum][id].options()[i].second.remove(usernum);
                                    change = true;
                                }

                        data[gidnum][id].options()[option - 1].second.insert(usernum);
                        message = QString("%1 %2 in poll#%3 (\"%4\")").arg(messageProcessor->convertToName(usernum))
                                  .arg(change ? "changed vote" : "voted").arg(id)
                                  .arg(data[gidnum][id].options()[option - 1].first);
                    }
                    else
                        message = QString("%1 has already voted for this option.")
                                  .arg(messageProcessor->convertToName(usernum));
                }
            }
            else if (args[1].toLower().startsWith("unvote") && args.size() > 2)
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
                                if (data[gidnum][id].options()[i].second.contains(usernum))
                                {
                                    data[gidnum][id].options()[i].second.remove(usernum);
                                    unvoted = i;
                                    break;
                                }
                        }
                        else
                        {
                            if (data[gidnum][id].options()[option - 1].second.contains(usernum))
                            {
                                unvoted = option - 1;
                                data[gidnum][id].options()[option - 1].second.remove(usernum);
                            }
                        }

                        if (unvoted != -1)
                            message = QString("%1 unvoted in poll#%2 (\"%3\")")
                                      .arg(messageProcessor->convertToName(usernum))
                                      .arg(id).arg(data[gidnum][id].options()[unvoted].first);
                        else
                            message = QString("%1 hasn't voted for this option.")
                                      .arg(messageProcessor->convertToName(usernum));
                    }
                }
            }
            else if (args[1].toLower().startsWith("who") && args.size() > 3)
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

            if (!message.isNull())
            {
                QByteArray sendee = inpm ? uid.toUtf8() : gid.toUtf8();
                messageProcessor->sendCommand("msg " + sendee + " \"" + message.replace('"', "\\\"").toUtf8() + '"');
            }
        }
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
