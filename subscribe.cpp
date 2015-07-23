#include "subscribe.h"
#include "database.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include "permission.h"
#include <QtCore>
#include <QSqlQuery>

Subscribe::Subscribe(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), permission(perm)
{
    loadData();
}

void Subscribe::loadData()
{
    data.clear();

    QSqlQuery query;
    query.prepare("SELECT gid, uid FROM tf_subscribe");
    database->executeQuery(query);

    while (query.next())
        data[query.value(0).toLongLong()].append(query.value(1).toLongLong());
}

void Subscribe::groupDeleted(qint64 gid)
{
    data.remove(gid);
}

void Subscribe::addUser(qint64 gid, qint64 uid)
{
    QSqlQuery query;
    query.prepare("INSERT INTO tf_subscribe (gid, uid) VALUES(:gid, :uid)");
    query.bindValue(":gid", gid);
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    data[gid].append(uid);
}

void Subscribe::delUser(qint64 gid, qint64 uid)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_subscribe WHERE gid=:gid AND uid=:uid");
    query.bindValue(":gid", gid);
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    data[gid].removeAll(uid);
}

void Subscribe::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!subscribe"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;
        int perm = 0;

        if (args.size() > 1 && args[1].toLower().startsWith("unsub"))
        {
            perm = permission->getPermission(gidnum, "subscribe", "unsubscribe", isAdmin, inpm);
            if (perm == 1)
            {
                if (data[gidnum].contains(uidnum))
                {
                    delUser(gidnum, uidnum);
                    message = QString("%1 unsubscribed successfully!").arg(messageProcessor->convertToName(uidnum));
                }
                else
                    message = QString("%1 is not subscribed!").arg(messageProcessor->convertToName(uidnum));
            }
        }
        else if (args.size() > 1 && args[1].toLower().startsWith("sub"))
        {
            perm = permission->getPermission(gidnum, "subscribe", "subscribe", isAdmin, inpm);
            if (perm == 1)
            {
                if (!data[gidnum].contains(uidnum))
                {
                    addUser(gidnum, uidnum);
                    message = QString("%1 subscribed successfully!").arg(messageProcessor->convertToName(uidnum));
                }
                else
                    message = QString("%1 is already subscribed!").arg(messageProcessor->convertToName(uidnum));
            }
        }
        else
        {
            perm = permission->getPermission(gidnum, "subscribe", "view", isAdmin, inpm);
            if (perm == 1)
            {
                if (data[gidnum].isEmpty())
                    message = "Noone!";
                else
                {
                    int i = 0;
                    foreach (qint64 id, data[gidnum])
                    {
                        message += QString("%1- %2").arg(i + 1).arg(messageProcessor->convertToName(id));

                        if (i != data[gidnum].size() - 1)
                            message += "\n";

                        ++i;
                    }
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}

void Subscribe::postToSubscribed(qint64 gid, const QString &str)
{
    QString message = QString("Notification from \"%1\":\n%2").arg(nameDatabase->groups()[gid].second)
                                                               .arg(str);

    foreach (qint64 uid, data[gid])
        messageProcessor->sendMessage("user#" + QString::number(uid), message);
}

void Subscribe::postToSubscribedAdmin(qint64 gid, const QString &str)
{
    qint64 adminid = nameDatabase->groups()[gid].first;

    if (data[gid].contains(adminid))
    {
        QString message = QString("Notification from \"%1\":\n%2").arg(nameDatabase->groups()[gid].second)
                                                                   .arg(str);

        messageProcessor->sendMessage("user#" + QString::number(adminid), message);
    }
}
