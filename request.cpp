#include "request.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include "subscribe.h"
#include <QtCore>
#include <QSqlQuery>

const int Request::MaxRequestsPerUserInGroup = 10;

Request::Request(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent)
    : QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

void Request::loadData()
{
    data.clear();

    QSqlQuery query;
    query.prepare("SELECT id, gid, uid, inpm, command FROM tf_requests");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 id = query.value(0).toLongLong();
        qint64 gid = query.value(1).toLongLong();
        qint64 uid = query.value(2).toLongLong();
        bool inpm = query.value(3).toBool();
        QString cmd = query.value(4).toString();

        data[gid].append(RequestEntry(id, uid, inpm, cmd));
    }
}

void Request::groupDeleted(qint64 gid)
{
    data.remove(gid);
}

void Request::addRequest(const QString &gid, const QString &uid, const QString &cmd, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    int reqs = 0;
    foreach (RequestEntry req, data[gidnum])
        if (req.uid() == uidnum)
            ++reqs;

    QString message;

    if (reqs < MaxRequestsPerUserInGroup)
    {
        QSqlQuery query;
        query.prepare("INSERT INTO tf_requests (gid, uid, inpm, command) VALUES "
                      "(:gid, :uid, :inpm, :command)");
        query.bindValue(":gid", gidnum);
        query.bindValue(":uid", uidnum);
        query.bindValue(":inpm", inpm);
        query.bindValue(":command", cmd);
        database->executeQuery(query);

        data[gidnum].append(RequestEntry(query.lastInsertId().toLongLong(), uidnum, inpm, cmd));

        if (data[gidnum].size() == 1)
            subscribe->postToSubscribedAdmin(gidnum, "There are new requests to review. Use !request to see.");

        message = "Command sent to be reviewed by admin!";
    }
    else
        message = "You cannot send request until your current requests get reviewed!";

    if (!message.isEmpty())
        messageProcessor->sendCommand("msg " + (inpm ? uid.toUtf8() : gid.toUtf8()) + " \"" +
                                      message.replace('"', "\\\"").toUtf8() + '"');
}

void Request::deleteRequest(qint64 gid, int id)
{
    qint64 reqid = data[gid][id - 1].id();

    QSqlQuery query;
    query.prepare("DELETE FROM tf_requests WHERE id=:id");
    query.bindValue(":id", reqid);
    database->executeQuery(query);

    data[gid].removeAt(id - 1);
}

void Request::processRequest(qint64 gid, int id)
{
    RequestEntry re = data[gid][id - 1];

    QString newgid = "chat#" + QString::number(gid);
    QString newuid = "user#" + QString::number(re.uid());

    messageProcessor->processCommand(newgid, newuid, re.command(), re.inpm(), true);
}

void Request::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum)
        && isAdmin && str.startsWith("!request"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 2 && (args[1].toLower().startsWith("ac") || args[1].toLower().startsWith("ax")
                                || args[1].toLower().startsWith("rej")))
        {
            bool isRej = args[1].toLower().startsWith("rej");

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
                for (int i = start; i <= end; ++i)
                {
                    if (!isRej)
                        processRequest(gidnum, start);
                    deleteRequest(gidnum, start);
                }

                message = QString("%1ed request entry(ies)!").arg(isRej ? "Reject" : "Accept");
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));

            if (data[gidnum].isEmpty())
                message = "Nothing to review!";
            else
            {
                for (int i = 0; i < data[gidnum].size(); ++i)
                {
                    message += QString("%1- %2 (By user#%3 - %4)%5").arg(i + 1)
                            .arg(data[gidnum][i].command())
                            .arg(data[gidnum][i].uid())
                            .arg(messageProcessor->convertToName(data[gidnum][i].uid()))
                            .arg(data[gidnum][i].inpm() ?  " (In PM)" : "");
                    if (i != data[gidnum].size() - 1)
                        message += "\n";
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);
    }
}
