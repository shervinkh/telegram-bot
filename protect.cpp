#include "protect.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include "smiley.h"
#include <QtCore>
#include <QSqlQuery>
#include <QSqlDatabase>

Protect::Protect(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent)
    : QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void Protect::loadData()
{
    data.clear();

    QSqlQuery query;
    query.prepare("SELECT gid, type, value FROM tf_protect");
    database->executeQuery(query);

    while (query.next())
        data[query.value(0).toLongLong()][query.value(1).toInt()] = query.value(2).toString();

    leaveData.clear();

    query.prepare("SELECT gid, uid FROM tf_leave_protect");
    database->executeQuery(query);

    while (query.next())
        leaveData[query.value(0).toLongLong()].insert(query.value(1).toLongLong());
}

void Protect::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum)
        && isAdmin && str.startsWith("!protect"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 1 && args[1].toLower().startsWith("leave"))
        {
            if (args.size() > 3 && (args[2].toLower().startsWith("add") || args[2].toLower().startsWith("del")
                                    || args[2].toLower().startsWith("rem")))
            {
                qint64 targetuid;
                bool ok;

                if (args[3].toLower().startsWith("all"))
                {
                    targetuid = -1;
                    ok = true;
                }
                else
                    targetuid = args[3].toLongLong(&ok);

                if (ok)
                {
                    if (args[2].toLower().startsWith("add"))
                    {
                        addLeaveProtect(gidnum, targetuid);
                        message = "Successfully added leave protect!";
                    }
                    else
                    {
                        delLeaveProtect(gidnum, targetuid);
                        message = "Successfully deleted leave protect!";
                    }
                }
            }
            else
            {
                if (leaveData[gidnum].isEmpty())
                    message = "Noone!";
                else
                {
                    int idx = 1;

                    foreach (qint64 tuid, leaveData[gidnum])
                    {
                        message += QString("%1- %2 (%3)").arg(idx).arg(tuid)
                                   .arg(messageProcessor->convertToName(tuid));

                        if (idx != leaveData[gidnum].size())
                            message += "\n";

                        ++idx;
                    }
                }
            }
        }
        else if (args.size() > 2 && (args[1].toLower().startsWith("name") || args[1].toLower().startsWith("photo")) &&
                 (args[2].toLower().startsWith("set") || args[2].toLower().startsWith("unset")))
        {
            int protectType = args[1].toLower().startsWith("photo");
            bool set = args[2].toLower().startsWith("set");

            if (set)
            {
                int idx = str.indexOf(' ', str.indexOf(args[2], 0, Qt::CaseInsensitive));
                while (str[idx] == ' ')
                    ++idx;

                QString value = str.mid(idx).trimmed();

                if (protectType == 0 || QFile::exists(value))
                {
                    setProtect(gidnum, protectType, value);
                    message = "Successfully set the protect!";
                }
            }
            else
            {
                unsetProtect(gidnum, protectType);
                message = "Successfully unset the protect!";
            }
        }
        else
        {
            message = "Current protects for this group:";
            if (data[gidnum].isEmpty())
                message += "\nNothing!";
            else
            {
                if (data[gidnum].contains(0))
                    message += QString("\nName Protection: %1").arg(data[gidnum][0]);

                if (data[gidnum].contains(1))
                    message += QString("\nPhoto Protection: %1").arg(data[gidnum][1]);
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);
    }
}

void Protect::rawInput(const QString &str)
{
    int idxOfChat = str.indexOf("chat#");
    int idxOfUser = str.indexOf("user#", idxOfChat + 1);

    if (idxOfChat != -1 && idxOfUser != -1)
    {
        qint64 gid = str.mid(idxOfChat + 5, str.indexOf('\e', idxOfChat + 5) - (idxOfChat + 5)).toLongLong();
        qint64 uid = str.mid(idxOfUser + 5, str.indexOf('\e', idxOfUser + 5) - (idxOfUser + 5)).toLongLong();

        if (uid != messageProcessor->botId())
        {
            int messageIdx = str.indexOf(' ', idxOfUser + 1);
            if (messageIdx != -1)
            {
                QString messageGot = str.mid(messageIdx).trimmed();

                if (messageGot.toLower().startsWith("changed title"))
                {
                    if (data.contains(gid) && data[gid].contains(0))
                    {
                        messageProcessor->sendCommand("rename_chat chat#" + QByteArray::number(gid)
                                                      + ' ' + data[gid][0].toUtf8());
                        messageProcessor->sendMessage("chat#" + QString::number(gid),
                                                      "Protected group against name change "
                                                      + Smiley::sunGlass + ". " + Smiley::pokerFace + " "
                                                      + messageProcessor->convertToName(uid));
                    }
                }
                else if (messageGot.toLower().startsWith("changed photo"))
                {
                    if (data.contains(gid) && data[gid].contains(1))
                    {
                        messageProcessor->sendCommand("chat_set_photo chat#" + QByteArray::number(gid)
                                                      + ' ' + data[gid][1].toUtf8());

                        messageProcessor->sendMessage("chat#" + QString::number(gid),
                                                      "Protected group against photo change "
                                                      + Smiley::sunGlass + ". " + Smiley::pokerFace + " "
                                                      + messageProcessor->convertToName(uid));
                    }
                }
                else if (messageGot.toLower().startsWith("deleted user"))
                {
                    int idxOfUser2 = str.indexOf("user#", idxOfUser + 1);

                    if (idxOfUser2 != -1)
                    {
                        qint64 uid2 = str.mid(idxOfUser2 + 5, str.indexOf('\e', idxOfUser2 + 5) - (idxOfUser2 + 5)).toLongLong();

                        if (leaveData[gid].contains(uid2))
                        {
                            messageProcessor->sendCommand("chat_add_user chat#" + QByteArray::number(gid)
                                                          + " user#" + QByteArray::number(uid2));
                            messageProcessor->sendMessage("chat#" + QString::number(gid),
                                                          QString("Protected %1 against leaving the group ")
                                                          .arg(messageProcessor->convertToName(uid2))
                                                          + Smiley::sunGlass + ". " + Smiley::pokerFace + " "
                                                          + messageProcessor->convertToName(uid));
                        }
                    }
                }
            }
        }
    }
}

void Protect::setProtect(qint64 gid, int type, const QString &value)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO tf_protect (gid, type) VALUES(:gid, :type)");
    query.bindValue(":gid", gid);
    query.bindValue(":type", type);
    database->executeQuery(query);

    query.prepare("UPDATE tf_protect SET value=:value WHERE gid=:gid AND type=:type");
    query.bindValue(":value", value);
    query.bindValue(":gid", gid);
    query.bindValue(":type", type);
    database->executeQuery(query);

    data[gid][type] = value;
}

void Protect::unsetProtect(qint64 gid, int type)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_protect WHERE gid=:gid AND type=:type");
    query.bindValue(":gid", gid);
    query.bindValue(":type", type);
    database->executeQuery(query);

    data[gid].remove(type);
}

void Protect::addLeaveProtect(qint64 gid, qint64 uid)
{
    QSqlQuery query;

    if (uid == -1)
    {
        QSqlDatabase::database().transaction();

        foreach (qint64 tuid, nameDatabase->userList(gid).keys())
        {
            leaveData[gid].insert(tuid);
            query.prepare("INSERT OR IGNORE INTO tf_leave_protect (gid, uid) VALUES (:gid, :uid)");
            query.bindValue(":gid", gid);
            query.bindValue(":uid", tuid);
            database->executeQuery(query);
        }

        QSqlDatabase::database().commit();
    }
    else
    {
        leaveData[gid].insert(uid);
        query.prepare("INSERT OR IGNORE INTO tf_leave_protect (gid, uid) VALUES (:gid, :uid)");
        query.bindValue(":gid", gid);
        query.bindValue(":uid", uid);
        database->executeQuery(query);
    }
}

void Protect::delLeaveProtect(qint64 gid, qint64 uid)
{
    QSqlQuery query;

    if (uid == -1)
    {
        leaveData[gid].clear();
        query.prepare("DELETE FROM tf_leave_protect WHERE gid=:gid");
        query.bindValue(":gid", gid);
    }
    else
    {
        leaveData[gid].remove(uid);
        query.prepare("DELETE FROM tf_leave_protect WHERE gid=:gid AND uid=:uid");
        query.bindValue(":gid", gid);
        query.bindValue(":uid", uid);
    }

    database->executeQuery(query);
}
