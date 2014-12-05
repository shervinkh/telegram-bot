#ifndef HELP_H
#define HELP_H

#include <QObject>

class MessageProcessor;

class Help : public QObject
{
    Q_OBJECT
private:
    MessageProcessor *messageProcessor;

public:
    explicit Help(MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &msg, bool inpm);
};

#endif // HELP_H
