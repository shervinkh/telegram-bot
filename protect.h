#ifndef PROTECT_H
#define PROTECT_H

#include <QObject>
#include <QMap>

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

    void loadData();
    void setProtect(qint64 gid, int type, const QString &value);
    void unsetProtect(qint64 gid, int type);

public:
    explicit Protect(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);
    void rawInput(const QString &str);

public slots:
};

#endif // PROTECT_H
