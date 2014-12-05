#include "subscribe.h"
#include "database.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include <QtCore>
#include <QSqlQuery>

Subscribe::Subscribe(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void Subscribe::loadData()
{
    QSqlQuery query;
    query.prepare("SELECT gid, uid FROM tf_subscribe");
    database->executeQuery(query);

    while (query.next())
        data[query.value(0).toLongLong()].append(query.value(1).toLongLong());
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

void Subscribe::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!subscribe"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;

        if (args.size() > 1 && args[1].toLower().startsWith("unsub"))
        {
            if (data[gidnum].contains(uidnum))
            {
                delUser(gidnum, uidnum);
                message = QString("%1 unsubscribed successfully!").arg(messageProcessor->convertToName(uidnum));
            }
            else
                message = QString("%1 is not subscribed!").arg(messageProcessor->convertToName(uidnum));
        }
        else if (args.size() > 1 && args[1].toLower().startsWith("sub"))
        {
            if (!data[gidnum].contains(uidnum))
            {
                addUser(gidnum, uidnum);
                message = QString("%1 subscribed successfully!").arg(messageProcessor->convertToName(uidnum));
            }
            else
                message = QString("%1 is already subscribed!").arg(messageProcessor->convertToName(uidnum));
        }
        else
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
                        message += "\\n";

                    ++i;
                }
            }
        }

        messageProcessor->sendCommand("msg " + (inpm ? uid.toUtf8() : gid.toUtf8()) + " \"" +
                                      message.replace('"', "\\\"").toUtf8() + '"');
    }
}

void Subscribe::postToSubscribed(qint64 gid, const QString &str)
{
    QString message = QString("Notification from \"%1\":\\n%2").arg(nameDatabase->groups()[gid].second)
                                                               .arg(str).replace('"', "\\\"");

    foreach (qint64 uid, data[gid])
        messageProcessor->sendCommand("msg user#" + QByteArray::number(uid) + " \""
                                      + message.toUtf8() + '"');
}
