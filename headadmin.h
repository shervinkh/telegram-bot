#ifndef HEADADMIN_H
#define HEADADMIN_H

#include <QObject>

class Database;
class NameDatabase;
class MessageProcessor;

class HeadAdmin : public QObject
{
    Q_OBJECT
private:
    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    void addMonitor(qint64 gid, qint64 aid, const QString &str);
    void delMonitor(qint64 gid);
    void changeAdmin(qint64 gid, qint64 aid);
    void changeName(qint64 gid, const QString &str);

public:
    explicit HeadAdmin(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);

public slots:
};

#endif // HEADADMIN_H
