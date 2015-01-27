#ifndef TREE_H
#define TREE_H

#include <QObject>
#include <QPair>

class NameDatabase;
class MessageProcessor;
class Permission;
class QTimer;

class Tree : public QObject
{
    Q_OBJECT
private:
    static const int TreeDelay;

    typedef QPair<qint64, QByteArray> QueueData;

    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;

    QList<QueueData> queue;

    QTimer *queueTimer;

public:
    explicit Tree(NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);

public slots:
    void processTree();
    void processQueue();
};

#endif // TREE_H
