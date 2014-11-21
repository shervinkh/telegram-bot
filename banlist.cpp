#include "banlist.h"
#include "messageprocessor.h"
#include "database.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include <QSqlQuery>
#include <QtCore>

BanList::BanList(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void BanList::loadData()
{
    QSqlQuery query;
    query.prepare("SELECT gid, uid FROM tf_banned");
    database->executeQuery(query);

    while (query.next())
        banned[query.value(0).toLongLong()].insert(query.value(1).toLongLong());
}

void BanList::addBan(qint64 gid, qint64 uid)
{
    QSqlQuery query;
    query.prepare("INSERT INTO tf_banned (gid, uid) VALUES(:gid, :uid)");
    query.bindValue(":gid", gid);
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    banned[gid].insert(uid);
}

void BanList::delBan(qint64 gid, qint64 uid)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_banned WHERE gid=:gid AND uid=:uid");
    query.bindValue(":gid", gid);
    query.bindValue(":uid", uid);
    database->executeQuery(query);

    banned[gid].remove(uid);
}

void BanList::input(const QString &gid, const QString &uid, const QString &str)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && nameDatabase->groups()[gidnum].first == uidnum
        && str.startsWith("!banlist"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;

        if (args.size() > 2 && (args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem")))
        {
            bool ok;
            qint64 uid = args[2].toLongLong(&ok);

            if (ok)
            {
                delBan(gidnum, uid);
                message = "Deleted from banlist " + messageProcessor->convertToName(uid) + "!";
            }
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("add"))
        {
            bool ok;
            qint64 uid = args[2].toLongLong(&ok);

            if (ok)
            {
                addBan(gidnum, uid);
                message = "Added to banlist " + messageProcessor->convertToName(uid) + "!";
            }
        }
        else
        {
            if (banned[gidnum].isEmpty())
                message = "Noone!";
            else
            {
                int i = 0;
                foreach (qint64 banid, banned[gidnum])
                {
                    message += QString("%1- %2").arg(i + 1).arg(messageProcessor->convertToName(banid));

                    if (i != banned[gidnum].size() - 1)
                        message += "\\n";

                    ++i;
                }
            }
        }

        messageProcessor->sendCommand("msg " + gid.toLatin1() + " \"" + message.toLatin1() + '"');
    }
}
