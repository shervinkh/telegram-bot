#include "messageprocessor.h"
#include "calculator.h"
#include "namedatabase.h"
#include "statistics.h"
#include "database.h"
#include "banlist.h"
#include "help.h"
#include "sup.h"
#include <QtCore>

const qint64 MessageProcessor::keepAliveInterval = 60000;

MessageProcessor::MessageProcessor(QObject *parent) :
    QObject(parent), output(stdout), endDayCron(-1), hourlyCron(-1)
{
    proc = new QProcess(this);
    proc->setReadChannel(QProcess::StandardOutput);
    QStringList args;
    args << "-C" << "-R" << "-I";

    connect(proc, SIGNAL(readyRead()), this, SLOT(readData()));
    proc->start(QCoreApplication::arguments()[1], args, QProcess::ReadWrite);

    keepAliveTimer = new QTimer(this);
    connect(keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
    keepAliveTimer->start(keepAliveInterval);

    calculator = new Calculator(this, this);

    database = new Database(&output, this);
    nameDatabase = new NameDatabase(database, this, this);
    stats = new Statistics(database, nameDatabase, this, this);
    help = new Help(this, this);
    sup = new Sup(database, nameDatabase, this, this);
    banlist = new BanList(database, nameDatabase, this, this);

    QTimer::singleShot(0, this, SLOT(keepAlive()));
}

MessageProcessor::~MessageProcessor()
{
    proc->terminate();
}

void MessageProcessor::readData()
{
    while (proc->canReadLine())
    {
        QString str = proc->readLine();
        nameDatabase->input(str);

        if (str.startsWith('['))
        {
            int idx = qMax(str.indexOf("»»»"), str.indexOf(">>>")) + 3;

            if (idx > 2)
            {
                QString cmd = str.mid(idx).trimmed();

                int chatidx = str.indexOf("chat#");
                int useridx = str.indexOf("user#");

                if (useridx == -1)
                    continue;

                QString gid, uid;
                if (chatidx != -1 && chatidx < useridx)
                    gid = str.mid(chatidx, 13);
                uid = str.mid(useridx, 13);

                QString identity = gid.isNull() ? uid : gid;

                output << "Got From " << identity << ": " << cmd << endl << flush;

                qint64 gidnum = gid.mid(5).toLongLong();
                qint64 uidnum = uid.mid(5).toLongLong();

                calculator->input(gid, uid, cmd);
                help->input(gid, uid, cmd);

                if (!banlist->bannedUsers()[gidnum].contains(uidnum))
                    sup->input(gid, uid, cmd);

                stats->input(gid, uid, cmd);

                banlist->input(gid, uid, cmd);
            }
        }
    }
}

void MessageProcessor::keepAlive()
{
    output << QString("Keeping alive... (%1)").arg(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"))
           << endl << flush;
    sendCommand("main_session");

    if (QTime::currentTime().hour() != hourlyCron)
    {
        output << "Running Hourly Cron..." << endl << flush;
        hourlyCron = QTime::currentTime().hour();
        nameDatabase->refreshDatabase();
    }

    if (endDayCron == -1)
        endDayCron = QDate::currentDate().toJulianDay();
    else if (QDate::currentDate().toJulianDay() != endDayCron)
    {
        output << "Running End-day Cron..." << endl << flush;
        endDayCron = QDate::currentDate().toJulianDay();
        foreach (qint64 gid, nameDatabase->groups().keys())
        {
            stats->giveStat(gid, "Yesterday", "summary");
            stats->giveStat(gid, "Yesterday", "maxcount", "all");
            stats->giveStat(gid, "Yesterday", "maxlength", "all");
        }
        stats->cleanUpBefore(QDate::currentDate().toJulianDay() - 3);
    }

    keepAliveTimer->start(keepAliveInterval);
}

void MessageProcessor::sendCommand(const QByteArray &str)
{
    proc->write(str + '\n');
}

qint64 MessageProcessor::processDate(const QString &str)
{
    if (str.trimmed().toLower() == "today")
        return QDate::currentDate().toJulianDay();
    else if (str.trimmed().toLower() == "yesterday")
        return QDate::currentDate().toJulianDay() - 1;
    else
        return QDate::fromString(str.trimmed(), "yyyy/MM/dd").toJulianDay();
}

QString MessageProcessor::convertToName(qint64 id)
{
    QString name;
    if (nameDatabase->nameDatabase().contains(id))
        name = nameDatabase->nameDatabase()[id];
    else
    {
        name = QString("user#%1").arg(id);
        nameDatabase->refreshDatabase();
    }

    return name;
}
