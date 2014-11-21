#ifndef BROADCAST_H
#define BROADCAST_H

#include <QObject>

class NameDatabase;
class MessageProcessor;

class Broadcast : public QObject
{
    Q_OBJECT
private:
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    void sendMessage(qint64 uid, QString str);

public:
    explicit Broadcast(NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str);

};

#endif // BROADCAST_H
