#ifndef GROUP_H
#define GROUP_H

#include <QObject>
#include <QMap>

class Database;
class NameDatabase;
class MessageProcessor;

class Group : public QObject
{
    Q_OBJECT
private:
    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    QMap<qint64, qint64> data;

    void loadData();
    void setGroup(qint64 uid, qint64 gid);
    void unsetGroup(qint64 uid);

public:
    explicit Group(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str);
    qint64 groupForUser(qint64 uid);
    void groupDeleted(qint64 gid);

public slots:

};

#endif // GROUP_H
