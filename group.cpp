#include "group.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QtCore>
#include <QtSql>

Group::Group(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void Group::loadData()
{
    QSqlQuery query;
    query.prepare("SELECT uid, gid FROM tf_usergroups");
    database->executeQuery(query);

    while (query.next())
        data[query.value(0).toLongLong()] = query.value(1).toLongLong();
}

void Group::input(const QString &gid, const QString &uid, const QString &str)
{
    qint64 uidnum = uid.mid(5).toLongLong();

    if (str.startsWith("!group"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;

        if (!gid.isEmpty())
            message = "Group module is not usable in groups!";
        else if (args.size() > 2 && args[1].toLower().startsWith("set"))
        {
            bool ok;
            qint64 id = args[2].toLongLong(&ok);

            if (ok && nameDatabase->groups().keys().contains(id) &&
                      nameDatabase->userList(id).contains(uidnum))
            {
                setGroup(uidnum, id);
                message = QString("Your group has been set to %1 (%2).").arg(id)
                                                               .arg(nameDatabase->groups()[id].second);
            }
        }
        else if (args.size() > 1 && args[1].toLower().startsWith("unset"))
        {
            unsetGroup(uidnum);
            message = "Your group has been unset.";
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("users"))
        {
            bool ok;
            qint64 id = args[2].toLongLong(&ok);

            if (ok && nameDatabase->groups().keys().contains(id) &&
                      nameDatabase->userList(id).contains(uidnum))
            {
                qint64 adminid = nameDatabase->groups()[id].first;

                int idx = 1;
                int size = nameDatabase->userList(id).size();

                foreach (qint64 curuid, nameDatabase->userList(id).keys())
                {
                    message += QString("%1- %2 (%3)%4").arg(idx).arg(curuid)
                                                       .arg(messageProcessor->convertToName(curuid))
                                                       .arg((curuid == adminid) ? " (Admin)" : "");
                    if (idx != size)
                        message += "\\n";

                    ++idx;
                }
            }
        }
        else
        {
            QList<qint64> gids;
            foreach (qint64 curgid, nameDatabase->groups().keys())
                if (nameDatabase->userList(curgid).keys().contains(uidnum))
                    gids.append(curgid);

            if (gids.isEmpty())
                message = "Your're not a memeber of groups I manage!";
            else
            {
                message = QString("Your groups:\\n");

                int idx = 1;
                foreach (qint64 curgid, gids)
                {
                    bool isThis = data.contains(uidnum) && (data[uidnum] == curgid);

                    message += QString("%1- %2 (%3)%4").arg(idx).arg(curgid)
                                                     .arg(nameDatabase->groups()[curgid].second)
                                                     .arg(isThis ? " (*)" : "");

                    if (idx != gids.size())
                        message += "\\n";

                    ++idx;
                }
            }
        }

        messageProcessor->sendCommand("msg " + (gid.isEmpty() ? uid.toUtf8() : gid.toUtf8()) + " \"" +
                                      message.toUtf8() + '"');
    }
}

void Group::setGroup(qint64 uid, qint64 gid)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO tf_usergroups (uid) VALUES(:uid)");
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    query.prepare("UPDATE tf_usergroups SET gid=:gid WHERE uid=:uid");
    query.bindValue(":gid", gid);
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    data[uid] = gid;
}

void Group::unsetGroup(qint64 uid)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_usergroups WHERE uid=:uid");
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    data.remove(uid);
}

qint64 Group::groupForUser(qint64 uid)
{
    if (data.contains(uid))
        return data[uid];
    return -1;
}
