#include "nickname.h"
#include "namedatabase.h"
#include "database.h"
#include "messageprocessor.h"
#include "permission.h"
#include <QtCore>
#include <QSqlQuery>

const int Nickname::MaxNicknamesPerPersonPerGroup = 5;

Nickname::Nickname(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Permission *perm, QObject *parent)
    : QObject(parent), nameDatabase(namedb), database(db), messageProcessor(msgproc), permission(perm)
{
    loadData();
}

void Nickname::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT gid, uid, nick FROM tf_nicks");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        qint64 uid = query.value(1).toLongLong();
        QByteArray nicks = query.value(2).toByteArray();

        QDataStream DS(nicks);
        Nicks parsedNicks;
        DS >> parsedNicks;

        data[gid][uid] = parsedNicks;
    }
}

void Nickname::saveData()
{
    QMapIterator<qint64, Users> groupsIter(data);
    while (groupsIter.hasNext())
    {
        groupsIter.next();
        QMapIterator<qint64, Nicks> usersIter(groupsIter.value());
        while (usersIter.hasNext())
        {
            usersIter.next();
            qint64 gid = groupsIter.key();
            qint64 uid = usersIter.key();
            Nicks nicks = usersIter.value();

            QByteArray rawNicks;
            QDataStream DS(&rawNicks, QIODevice::Append);
            DS << nicks;

            QSqlQuery query;
            query.prepare("INSERT OR IGNORE INTO tf_nicks (gid, uid) "
                          "VALUES(:gid, :uid)");
            query.bindValue(":gid", gid);
            query.bindValue(":uid", uid);
            database->executeQuery(query);

            query.prepare("UPDATE tf_nicks SET nick=:nick WHERE gid=:gid AND "
                          "uid=:uid");
            query.bindValue(":nick", rawNicks);
            query.bindValue(":gid", gid);
            query.bindValue(":uid", uid);
            database->executeQuery(query);
        }
    }
}

void Nickname::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!nick"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;
        int perm = 0;

        if (args.size() > 3 && args[1].toLower().startsWith("add"))
        {
            perm = permission->getPermission(gidnum, "nick", "add", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok;
                qint64 uid = args[2].toLongLong(&ok);

                if (ok)
                {
                    int idx = str.indexOf(' ', str.indexOf(args[2]));
                    QString name = str.mid(idx + 1).trimmed();

                    if (data[gidnum][uid].size() < MaxNicknamesPerPersonPerGroup)
                    {
                        addNick(gidnum, uid, name);
                        message = "Added new nickname for " + messageProcessor->convertToName(uid) + "!";
                    }
                    else
                        message = "Too much nicknames for this user!";
                }
            }
        }
        else if (args.size() > 3 && (args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem")))
        {
            perm = permission->getPermission(gidnum, "nick", "delete", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok1, ok2;
                qint64 uid = args[2].toLongLong(&ok1);
                int idx = args[3].toInt(&ok2);

                if (ok1 && ok2 && idx > 0 && idx <= data[gidnum][uid].size())
                {
                    delNick(gidnum, uid, idx);
                    message = "Deleted nickname for " + messageProcessor->convertToName(uid) + "!";
                }
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args.last().toLower().startsWith("pm"));

            perm = permission->getPermission(gidnum, "nick", "view", isAdmin, inpm);

            bool ok = false;
            int uid;
            if (args.size() > 1)
                uid = args[1].toInt(&ok);

            if (perm == 1)
            {
                if (ok)
                {
                    if (data[gidnum].contains(uid))
                    {
                        message = QString("%1 (%2): ").arg(uid).arg(messageProcessor->convertToName(uid));

                        int idx = 0;
                        foreach (QString nick, data[gidnum][uid])
                        {
                            message += QString("\"%1\"").arg(nick);
                            if (++idx != data[gidnum][uid].size())
                                message += ", ";
                        }
                    }
                    else
                        message = "Nothing!";
                }
                else
                {
                    if (data[gidnum].isEmpty())
                        message = "Noone!";
                    else
                    {
                        int i = 0;
                        foreach (qint64 uid, data[gidnum].keys())
                        {
                            message += QString("%1 (%2): ").arg(uid).arg(messageProcessor->convertToName(uid));

                            int j = 0;
                            foreach (QString nick, data[gidnum][uid])
                            {
                                message += QString("\"%1\"").arg(nick);
                                if (++j != data[gidnum][uid].size())
                                    message += ", ";
                            }

                            if (++i != data[gidnum].size())
                                message += '\n';
                        }
                    }
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}

void Nickname::addNick(qint64 gid, qint64 uid, const QString &str)
{
    data[gid][uid].append(str);
}

void Nickname::delNick(qint64 gid, qint64 uid, int idx)
{
    data[gid][uid].removeAt(idx - 1);

    if (data[gid][uid].isEmpty())
    {
        data[gid].remove(uid);
        QSqlQuery query;
        query.prepare("DELETE FROM tf_nicks WHERE gid=:gid AND uid=:uid");
        query.bindValue(":gid", gid);
        query.bindValue(":uid", uid);
        database->executeQuery(query);
    }
}
