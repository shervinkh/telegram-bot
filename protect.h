#ifndef PROTECT_H
#define PROTECT_H

#include <QObject>
#include <QMap>
#include <QSet>

class Database;
class NameDatabase;
class MessageProcessor;

class Protect : public QObject
{
    Q_OBJECT
private:
    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    typedef QMap<int, QString> GroupData;
    QMap<qint64, GroupData> data;

    typedef QSet<qint64> UsersList;
    QMap<qint64, UsersList> leaveData;

    void loadData();
    void setProtect(qint64 gid, int type, const QString &value);
    void unsetProtect(qint64 gid, int type);

    void addLeaveProtect(qint64 gid, qint64 uid);
    void delLeaveProtect(qint64 gid, qint64 uid);

public:
    explicit Protect(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);
    void rawInput(const QString &str);

public slots:
};

#endif // PROTECT_H
