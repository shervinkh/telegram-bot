#include "tree.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QtCore>

Tree::Tree(NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgproc), lastGid(-1)
{
}

void Tree::input(const QString &gid, const QString &, const QString &str)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!tree") && lastGid == -1)
    {
        QFile file("tree.dot");

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

            lastGid = gidnum;

            QProcess *graphviz = new QProcess(this);
            connect(graphviz, SIGNAL(finished(int)), this, SLOT(processTree()));

            QStringList args;
            args << "-Tpng" << "tree.dot";

            graphviz->setStandardOutputFile("tree.png");

            graphviz->start("dot", args);
        }
    }
}

void Tree::processTree()
{
    QProcess *proc = qobject_cast<QProcess *>(sender());

    if (proc->exitCode() == 0)
        messageProcessor->sendCommand("send_document chat#" + QByteArray::number(lastGid) + " tree.png");

    proc->deleteLater();
    lastGid = -1;
}
