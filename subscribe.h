#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include <QObject>
#include <QMap>

class Database;
class NameDatabase;
class MessageProcessor;

class Subscribe : public QObject
{
    Q_OBJECT
private:
    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    typedef QList<qint64> UsersList;
    QMap<qint64, UsersList> data;

    void loadData();
    void delUser(qint64 gid, qint64 uid);
    void addUser(qint64 gid, qint64 uid);

public:
    explicit Subscribe(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);
    void postToSubscribed(qint64 gid, const QString &str);

public slots:

};

#endif // SUBSCRIBE_H
