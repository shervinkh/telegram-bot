#include "namedatabase.h"
#include "messageprocessor.h"
#include "database.h"
#include <QtCore>
#include <QSqlQuery>

NameDatabase::NameDatabase(Database *db, MessageProcessor *mp, QObject *parent) :
    QObject(parent), messageProcessor(mp), database(db)
{
    QSqlQuery query;
    query.prepare("SELECT gid, admin_id, name FROM tf_groups");
    database->executeQuery(query);

    while (query.next())
        monitoringGroups[query.value(0).toLongLong()] = GroupData(query.value(1).toLongLong(),
                                                                  query.value(2).toString());
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
    if (str.startsWith("\t\t"))
    {
        qint64 user = str.mid(7, 8).toLongLong();
        qint64 parent = str.mid(32, 8).toLongLong();

        ids[lastGidSeen][user] = parent;
        if (str.indexOf("admin") != -1)
            getNames(lastGidSeen);
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
            lastUidSeen = str.mid(uididx + 5, 8).toLongLong();

        int gididx = str.indexOf("chat#");
        if (gididx != -1)
            lastGidSeen = str.mid(gididx + 5, 8).toLongLong();
    }
}

void NameDatabase::getNames(qint64 gid)
{
    foreach (qint64 id, ids[gid].keys())
        messageProcessor->sendCommand("user_info user#" + QByteArray::number(id));
}
