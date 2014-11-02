#include "namedatabase.h"
#include "messageprocessor.h"
#include "database.h"
#include <QtCore>
#include <QSqlQuery>

NameDatabase::NameDatabase(Database *db, MessageProcessor *mp, QObject *parent) :
    QObject(parent), messageProcessor(mp), database(db)
{
    QSqlQuery query;
    query.prepare("SELECT gid FROM tf_groups");
    database->executeQuery(query);

    while (query.next())
        monitoringGroups.insert(query.value(0).toLongLong());
}

void NameDatabase::refreshDatabase()
{
    foreach (qint64 gid, monitoringGroups)
        messageProcessor->sendCommand("chat_info chat#" + QByteArray::number(gid));
}

void NameDatabase::input(const QString &str)
{
    if (str.startsWith("\t\t"))
    {
        ids.insert(str.mid(7, 8).toLongLong());
        if (str.indexOf("admin") != -1)
            getNames();
    }
    else
    {
        int idx = str.indexOf('#');
        if (idx != -1)
            lastIdSeen = str.mid(idx + 1, 8).toLongLong();

        if (str.startsWith("\t"))
        {
            int idx = str.indexOf("real name: ");
            if (idx != -1)
            {
                QString name = str.mid(idx + QString("real name: ").size()).trimmed();
                data[lastIdSeen] = name;
            }
        }
    }
}

void NameDatabase::getNames()
{
    foreach (qint64 id, ids)
        messageProcessor->sendCommand("user_info user#" + QByteArray::number(id));
}
