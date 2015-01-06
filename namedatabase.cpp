#include "namedatabase.h"
#include "messageprocessor.h"
#include "database.h"
#include <QtCore>
#include <QSqlQuery>

NameDatabase::NameDatabase(Database *db, MessageProcessor *mp, QObject *parent) :
    QObject(parent), messageProcessor(mp), database(db), gettingList(false)
{
    loadData();
}

void NameDatabase::loadData()
{
    monitoringGroups.clear();

    QSqlQuery query;
    query.prepare("SELECT gid, admin_id, name FROM tf_groups");
    database->executeQuery(query);

    while (query.next())
        monitoringGroups[query.value(0).toLongLong()] = GroupData(query.value(1).toLongLong(),
                                                                  query.value(2).toString());

    refreshDatabase();
}

void NameDatabase::refreshDatabase()
{
    foreach (qint64 gid, monitoringGroups.keys())
    {
        messageProcessor->sendCommand("chat_info chat#" + QByteArray::number(gid));
        ids[gid].clear();
    }
}

void NameDatabase::input(const QString &str)
{
    int idxOfChat = str.indexOf("chat#");
    if (idxOfChat != -1)
    {
        int idxOfUser = str.indexOf("user#", idxOfChat + 1);
        if (idxOfUser != -1)
        {
            int idxOfMessage = str.indexOf(' ', idxOfUser + 1);
            QString message = str.mid(idxOfMessage).trimmed();
            if (message.toLower().startsWith("added user") || message.toLower().startsWith("deleted user"))
                refreshDatabase();
        }
    }

    if (gettingList && !str.startsWith("\t\t"))
    {
        gettingList = false;
        getNames(lastGidSeen);
    }

    if (str.startsWith("\t\t"))
    {
        gettingList = true;

        int firstIdx = str.indexOf("user#");
        int secondIdx = str.indexOf("user#", firstIdx + 1);
        qint64 user = str.mid(firstIdx + 5, str.indexOf(' ', firstIdx) - (firstIdx + 5)).toLongLong();
        qint64 parent = str.mid(secondIdx + 5, str.indexOf(' ', secondIdx) - (secondIdx + 5)).toLongLong();

        ids[lastGidSeen][user] = parent;
    }
    else if (str.startsWith("\t"))
    {
        int idx = str.indexOf("real name: ");
        if (idx != -1)
        {
            QString name = str.mid(idx + QString("real name: ").size()).trimmed();
            data[lastUidSeen] = name;
        }
    }
    else
    {
        int uididx = str.indexOf("user#");
        if (uididx != -1)
            lastUidSeen = str.mid(uididx + 5, str.indexOf(' ', uididx) - (uididx + 5)).toLongLong();

        int gididx = str.indexOf("chat#");
        if (gididx != -1)
            lastGidSeen = str.mid(gididx + 5, str.indexOf(' ', gididx) - (gididx + 5)).toLongLong();
    }
}

void NameDatabase::getNames(qint64 gid)
{
    foreach (qint64 id, ids[gid].keys())
        messageProcessor->sendCommand("user_info user#" + QByteArray::number(id));
}
