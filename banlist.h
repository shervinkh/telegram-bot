#ifndef BANLIST_H
#define BANLIST_H

#include <QObject>
#include <QMap>
#include <QSet>

class Database;
class NameDatabase;
class MessageProcessor;
class Permission;

class BanList : public QObject
{
    Q_OBJECT
private:
    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;

    typedef QSet<qint64> Users;
    QMap<qint64, Users> banned;

    void loadData();
    void addBan(qint64 gid, qint64 uid);
    void delBan(qint64 gid, qint64 uid);

public:
    explicit BanList(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm,
                     QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);
    const QMap<qint64, Users> &bannedUsers() const {return banned;}
    void groupDeleted(qint64 gid);

public slots:

};

#endif // BANLIST_H
