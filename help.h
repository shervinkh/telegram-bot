#ifndef HELP_H
#define HELP_H

#include <QObject>

class NameDatabase;
class MessageProcessor;
class Permission;

class Help : public QObject
{
    Q_OBJECT
private:
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;

public:
    explicit Help(NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &msg, bool inpm, bool isAdmin);
};

#endif // HELP_H
