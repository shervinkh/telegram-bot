#include "calculator.h"
#include "messageprocessor.h"
#include <QtCore>

const qint64 Calculator::timeLimit = 2000;

Calculator::Calculator(MessageProcessor *msgProc, QObject *parent) :
    QObject(parent), messageProcessor(msgProc)
{
    checkEndTimer = new QTimer(this);
    connect(checkEndTimer, SIGNAL(timeout()), this, SLOT(checkEndTime()));
    checkEndTimer->start(timeLimit);
}

void Calculator::input(const QString &gid, const QString &uid, const QString &msg)
{
    if (msg.startsWith("!calc"))
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

                QString identity = gid.isNull() ? uid : gid;
                id[proc] = identity.toUtf8();
            }
        }
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

    qDebug() << answer.length() << " Ready ";

    QByteArray cmd = "msg " + id[pyProc] + " \"The Answer Is: "
                     + answer.trimmed().replace('\n', "\\n").replace('"', "\\\"") + '"';
    messageProcessor->sendCommand(cmd);

    endTime.remove(pyProc);
    id.remove(pyProc);
    pyProc->deleteLater();
}
