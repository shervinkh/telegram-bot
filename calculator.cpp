#include "calculator.h"
#include "messageprocessor.h"
#include "permission.h"
#include "namedatabase.h"
#include <QtCore>

const qint64 Calculator::timeLimit = 2000;

Calculator::Calculator(NameDatabase *namedb, MessageProcessor *msgProc, Permission *perm, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgProc), permission(perm)
{
    checkEndTimer = new QTimer(this);
    connect(checkEndTimer, SIGNAL(timeout()), this, SLOT(checkEndTime()));
    checkEndTimer->start(timeLimit);
}

void Calculator::input(const QString &gid, const QString &uid, const QString &msg, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (msg.startsWith("!calc"))
    {
        int perm = permission->getPermission(gidnum, "calc", "use_in_group", isAdmin, inpm);
        if (inpm || perm == 1)
        {
            int idx = msg.indexOf(' ');

            if (idx != -1)
            {
                while (idx < msg.size() && msg[idx] == ' ')
                    ++idx;

                if (idx < msg.size())
                {
                    QString cmd = msg.mid(idx).trimmed().replace('"', "\\\"");

                    QStringList args;
                    args << "-u" << "noone" << "./run.sh"
                         << QString("from math import *; from random import *; print(%1)").arg(cmd);

                    QProcess *proc = new QProcess(this);
                    connect(proc, SIGNAL(finished(int)), this, SLOT(processCalculator()));
                    proc->start("sudo", args);

                    endTime[proc] = QDateTime::currentMSecsSinceEpoch() + timeLimit;

                    id[proc] = inpm ? uid : gid;
                }
            }
        }
        else if (perm == 2)
            permission->sendRequest(gid, uid, msg, inpm);
    }
}

void Calculator::checkEndTime()
{
    QMapIterator<QProcess *, qint64> iter(endTime);

    while (iter.hasNext())
    {
        iter.next();

        if (QDateTime::currentMSecsSinceEpoch() >= iter.value())
        {
            QStringList args;
            args << "-u" << "noone" << "pkill" << "-P" << iter.key()->readAllStandardError().trimmed();
            QProcess::execute("sudo", args);
        }
    }

    checkEndTimer->start(timeLimit);
}

void Calculator::processCalculator()
{
    QProcess *pyProc = qobject_cast<QProcess *>(sender());
    QByteArray answer = pyProc->readAllStandardOutput();

    messageProcessor->sendMessage(id[pyProc], "The Answer Is: " + answer.trimmed());

    endTime.remove(pyProc);
    id.remove(pyProc);
    pyProc->deleteLater();
}
