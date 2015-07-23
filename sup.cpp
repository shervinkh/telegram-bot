#include "sup.h"
#include "database.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include "subscribe.h"
#include "permission.h"
#include "smiley.h"
#include <QtCore>
#include <QSqlQuery>

const int Sup::MaxSupPerGroup = 10;
const int Sup::DistanceBetweenSup = 30;

Sup::Sup(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, Subscribe *sub, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), permission(perm), subscribe(sub)
{
    loadData();
    freshSup();
}

void Sup::freshSup()
{
    foreach (qint64 gid, nameDatabase->groups().keys())
        lastSup[gid] = DistanceBetweenSup;
}

void Sup::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT id, gid, text FROM tf_sup");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 id = query.value(0).toLongLong();
        qint64 gid = query.value(1).toLongLong();
        QString text = query.value(2).toString();

        data[gid].append(SupEntry(id, text));
    }
}

void Sup::groupDeleted(qint64 gid)
{
    data.remove(gid);
}

void Sup::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
        ++lastSup[gidnum];

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!sup"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;
        int perm = 0;

        if (args.size() > 2 && (args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem")))
        {
            perm = permission->getPermission(gidnum, "sup", "delete", isAdmin, inpm);
            if (perm == 1)
            {
                int start, end;
                int idx = args[2].indexOf('-');
                bool ok1, ok2;

                if (idx == -1)
                {
                    start = end = args[2].toInt(&ok1);
                    ok2 = true;
                }
                else
                {
                    start = args[2].mid(0, idx).toInt(&ok1);
                    end = args[2].mid(idx + 1).toInt(&ok2);
                }

                if (ok1 && ok2 && start > 0 && end > 0 && end >= start && end <= data[gidnum].size())
                {
                    qDebug() << "Deleting: " << start << ' ' << end;
                    for (int i = start; i <= end; ++i)
                        delEntry(gidnum, start);
                    message = "Deleted 'sup entry(ies)!";
                }
            }
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("add"))
        {
            perm = permission->getPermission(gidnum, "sup", "add", isAdmin, inpm);
            if (perm == 1)
            {
                int idx = str.indexOf("add", 0, Qt::CaseInsensitive);
                int seenSpace = 0;
                while (idx < str.length())
                {
                    if (str[idx] == ' ')
                        seenSpace = 1;
                    else if (seenSpace == 1)
                    {
                        seenSpace = 2;
                        break;
                    }

                    ++idx;
                }

                if (seenSpace == 2)
                {
                    if (addEntry(gidnum, str.mid(idx)))
                    {
                        qDebug() << "Here adding sup...!";
                        message = "Added 'sup entry!";
                        freshSup();

                        QString subscribeMessage = QString("New 'Sup Entry: %1").arg(str.mid(idx));
                        subscribe->postToSubscribed(gidnum, subscribeMessage);
                    }
                    else
                        message = "Couldn't add new 'sup entry; Maybe delete some entries first.";
                }
            }
        }
        else if (args.size() > 3 && args[1].toLower().startsWith("edit"))
        {
            perm = permission->getPermission(gidnum, "sup", "edit", isAdmin, inpm);
            if (perm == 1)
            {
                bool ok;
                int id = args[2].toInt(&ok);

                if (ok && id > 0 && id <= data[gidnum].size())
                {
                    int idx = str.indexOf("edit", 0, Qt::CaseInsensitive);
                    while (str[idx] != ' ')
                        ++idx;
                    while (str[idx] == ' ')
                        ++idx;
                    while (str[idx] != ' ')
                        ++idx;
                    while (str[idx] == ' ')
                        ++idx;

                    editEntry(gidnum, id, str.mid(idx));

                    message = "Edited 'sup entry!";
                }
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));

            perm = permission->getPermission(gidnum, "sup", "view", isAdmin, inpm);
            if (perm == 1)
            {
                if (data[gidnum].isEmpty())
                    message = "Nothing important!";
                else
                {
                    if (!inpm && lastSup[gidnum] < DistanceBetweenSup)
                        message = "Public 'sup have been already shown recently. Please scroll up or use \"!sup pm\".";
                    else
                    {
                        for (int i = 0; i < data[gidnum].size(); ++i)
                        {
                            message += QString("%1- %2").arg(i + 1).arg(data[gidnum][i].text());
                            if (i != data[gidnum].size() - 1)
                                message += "\n";
                        }

                        if (!inpm)
                        {
                            lastSup[gidnum] = 0;
                            message += QString("\n\n") + Smiley::pokerFace;
                        }
                    }
                }
            }
        }

        qDebug() << "sup module send message: " << (inpm ? uid : gid) << message;
        messageProcessor->sendMessage(inpm ? uid : gid, message);

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}

bool Sup::addEntry(qint64 gid, const QString &str)
{
    if (data[gid].size() < MaxSupPerGroup)
    {
        QSqlQuery query;
        query.prepare("INSERT INTO tf_sup (gid, text) VALUES(:gid, :text)");
        query.bindValue(":gid", gid);
        query.bindValue(":text", str);
        database->executeQuery(query);
        qint64 rid = query.lastInsertId().toLongLong();

        if (rid != -1)
        {
            data[gid].append(SupEntry(rid, str));
            return true;
        }
    }

    return false;
}

void Sup::delEntry(qint64 gid, int id)
{
    if (id <= data[gid].size())
    {
        int realId = data[gid][id - 1].Id();

        QSqlQuery query;
        query.prepare("DELETE FROM tf_sup WHERE id=:id");
        query.bindValue(":id", realId);
        database->executeQuery(query);

        data[gid].removeAt(id - 1);
    }
}

void Sup::editEntry(qint64 gid, int id, const QString &str)
{
    if (id <= data[gid].size())
    {
        int realId = data[gid][id - 1].Id();

        QSqlQuery query;
        query.prepare("UPDATE tf_sup SET text=:text WHERE id=:id");
        query.bindValue(":text", str);
        query.bindValue(":id", realId);
        database->executeQuery(query);

        data[gid][id - 1].text() = str;
    }
}
