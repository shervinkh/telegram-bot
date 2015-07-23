#ifndef REQUEST_H
#define REQUEST_H

#include <QObject>
#include <QMap>

class Database;
class NameDatabase;
class MessageProcessor;
class Subscribe;

class RequestEntry
{
private:
    qint64 i;
    qint64 u;
    bool pm;
    QString cmd;

public:
    RequestEntry() {}
    RequestEntry(qint64 id, qint64 uid, bool inpm, const QString &command)
        : i(id), u(uid), pm(inpm), cmd(command) {}

    qint64 id() const {return i;}
    qint64 uid() const {return u;}
    bool inpm() const {return pm;}
    QString command() const {return cmd;}
};

class Request : public QObject
{
    Q_OBJECT
private:
    static const int MaxRequestsPerUserInGroup;

    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Subscribe *subscribe;

    typedef QList<RequestEntry> Requests;
    QMap<qint64, Requests> data;

    void loadData();
    void deleteRequest(qint64 gid, int id);
    void processRequest(qint64 gid, int id);

public:
    explicit Request(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void addRequest(const QString &gid, const QString &uid, const QString &cmd, bool inpm);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);
    void setSubscribe(Subscribe *sub) {subscribe = sub;}
    void groupDeleted(qint64 gid);

public slots:
};

#endif // REQUEST_H
