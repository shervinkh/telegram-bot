#include "tree.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include "permission.h"
#include <QtCore>

const int Tree::TreeDelay = 10000;

Tree::Tree(NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgproc), permission(perm)
{
    queueTimer = new QTimer(this);
    queueTimer->setInterval(TreeDelay);
    connect(queueTimer, SIGNAL(timeout()), this, SLOT(processQueue()));
}

void Tree::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!tree"))
    {
        int perm = permission->getPermission(gidnum, "tree", "use", isAdmin, inpm);
        if (perm == 1)
        {
            QStringList args = str.split(' ', QString::SkipEmptyParts);
            inpm = inpm || (args.size() > 1 && args[1].toLower().startsWith("pm"));
            QByteArray identity = inpm ? uid.toUtf8() : gid.toUtf8();
            queue.push_back(QueueData(gidnum, identity));

            if (queue.size() == 1 && !queueTimer->isActive())
                processQueue();
        }
        else if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}

void Tree::processTree()
{
    QProcess *proc = qobject_cast<QProcess *>(sender());

    if (proc->exitCode() == 0)
        messageProcessor->sendCommand("send_document " + queue.front().second + " tree.png");

    proc->deleteLater();
    queue.pop_front();
    queueTimer->start();
}

void Tree::processQueue()
{
    if (queue.isEmpty())
    {
        queueTimer->stop();
        return;
    }

    QFile file("tree.dot");

    qint64 gidnum = queue.front().first;

    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream TS(&file);
        TS.setCodec(QTextCodec::codecForName("UTF-8"));

        TS << "digraph TelegramUsers" << endl << "{" << endl;

        QMapIterator<qint64, qint64> users(nameDatabase->userList(gidnum));
        while (users.hasNext())
        {
            users.next();
            TS << "    \"" << messageProcessor->convertToName(users.value()) << "\" -> \""
               <<  messageProcessor->convertToName(users.key()) << "\"" << endl;
        }

        TS << "}" << endl;

        QProcess *graphviz = new QProcess(this);
        connect(graphviz, SIGNAL(finished(int)), this, SLOT(processTree()));

        QStringList args;
        args << "-Tpng" << "tree.dot";

        graphviz->setStandardOutputFile("tree.png");

        graphviz->start("dot", args);
    }
    else
    {
        queue.pop_front();
        processQueue();
    }
}
