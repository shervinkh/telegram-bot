#ifndef NAMEDATABASE_H
#define NAMEDATABASE_H

#include <QObject>
#include <QMap>
#include <QSet>

class MessageProcessor;
class Database;

class NameDatabase : public QObject
{
    Q_OBJECT
public:
    typedef QMap<qint64, qint64> UserList;
    typedef QPair<qint64, QString> GroupData;

private:
    QMap<qint64, QString> data;
    QMap<qint64, UserList> ids;

    MessageProcessor *messageProcessor;
    Database *database;

    QMap<qint64, GroupData> monitoringGroups;

    qint64 lastGidSeen;
    qint64 lastUidSeen;

    bool gettingList;

    void refreshGroupNames(qint64 gid);
    void refreshGroup(qint64 gid);
    void refreshUser(qint64 uid);

public:
    explicit NameDatabase(Database *db, MessageProcessor *mp, QObject *parent = 0);
    void loadData();
    void input(const QString &str);
    void refreshDatabase();
    const QMap<qint64, GroupData> &groups() const {return monitoringGroups;}
    const QMap<qint64, QString> &nameDatabase() const {return data;}
    UserList userList(qint64 gid) const {return ids[gid];}
};

#endif // NAMEDATABASE_H
