#include "protect.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QtCore>
#include <QSqlQuery>

Protect::Protect(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent)
    : QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void Protect::loadData()
{
    QSqlQuery query;
    query.prepare("SELECT gid, type, value FROM tf_protect");
    database->executeQuery(query);

    while (query.next())
        data[query.value(0).toLongLong()][query.value(1).toInt()] = query.value(2).toString();
}

void Protect::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum)
        && (nameDatabase->groups()[gidnum].first == uidnum || uidnum == messageProcessor->headAdminId())
        && str.startsWith("!protect"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 2 && (args[1].toLower().startsWith("name") || args[1].toLower().startsWith("photo")) &&
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
                message += "\\nNothing!";
            else
            {
                if (data[gidnum].contains(0))
                    message += QString("\\nName Protection: %1").arg(data[gidnum][0]);

                if (data[gidnum].contains(1))
                    message += QString("\\nPhoto Protection: %1").arg(data[gidnum][1]);
            }
        }

        if (!message.isEmpty())
            messageProcessor->sendCommand("msg " + (inpm ? uid.toUtf8() : gid.toUtf8()) + " \"" +
                                          message.replace('"', "\\\"").toUtf8() + '"');
    }
}

void Protect::rawInput(const QString &str)
{
    int idxOfChat = str.indexOf("chat#");
    int idxOfUser = str.indexOf("user#", idxOfChat + 1);

    if (idxOfChat != -1 && idxOfUser != -1)
    {
        qint64 gid = str.mid(idxOfChat + 5, str.indexOf(' ', idxOfChat + 5) - (idxOfChat + 5)).toLongLong();
        qint64 uid = str.mid(idxOfUser + 5, str.indexOf(' ', idxOfUser + 5) - (idxOfUser + 5)).toLongLong();

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
                        messageProcessor->sendCommand("msg chat#" + QByteArray::number(gid)
                                                      + " \"Protected group against name change. :| "
                                                      + messageProcessor->convertToName(uid).toUtf8() + '"');
                    }
                }
                else if (messageGot.toLower().startsWith("changed photo"))
                {
                    if (data.contains(gid) && data[gid].contains(1))
                    {
                        messageProcessor->sendCommand("chat_set_photo chat#" + QByteArray::number(gid)
                                                      + ' ' + data[gid][1].toUtf8());
                        messageProcessor->sendCommand("msg chat#" + QByteArray::number(gid)
                                                      + " \"Protected group against photo change. :| "
                                                      + messageProcessor->convertToName(uid).toUtf8() + '"');
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
