#ifndef NICKNAME_H
#define NICKNAME_H

#include <QObject>
#include <QMap>

class NameDatabase;
class Database;
class MessageProcessor;
class Permission;

class Nickname : public QObject
{
    Q_OBJECT
private:
    static const int MaxNicknamesPerPersonPerGroup;

    typedef QList<QString> Nicks;
    typedef QMap<qint64, Nicks> Users;
    typedef QMap<qint64, Users> Groups;

    Groups data;

    NameDatabase *nameDatabase;
    Database *database;
    MessageProcessor *messageProcessor;
    Permission *permission;

    void loadData();
    void addNick(qint64 gid, qint64 uid, const QString &str);
    void delNick(qint64 gid, qint64 uid, int idx);

public:
    explicit Nickname(NameDatabase *namedb, Database *db, MessageProcessor *msgproc,
                      Permission *perm, QObject *parent = 0);
    void saveData();
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);
    Nicks nickNames(qint64 gid, qint64 uid) {if (data[gid].contains(uid)) return data[gid][uid]; else return Nicks();}

public slots:
};

#endif // NICKNAME_H
