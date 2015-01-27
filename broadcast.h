#ifndef BROADCAST_H
#define BROADCAST_H

#include <QObject>

class NameDatabase;
class MessageProcessor;
class Permission;

class Broadcast : public QObject
{
    Q_OBJECT
private:
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;

public:
    explicit Broadcast(NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);

};

#endif // BROADCAST_H
