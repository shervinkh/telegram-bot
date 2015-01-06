#include "messageprocessor.h"
#include "calculator.h"
#include "namedatabase.h"
#include "statistics.h"
#include "broadcast.h"
#include "subscribe.h"
#include "headadmin.h"
#include "database.h"
#include "banlist.h"
#include "protect.h"
#include "group.h"
#include "help.h"
#include "poll.h"
#include "tree.h"
#include "sup.h"
#include <QtCore>
#include <QSqlQuery>

const qint64 MessageProcessor::keepAliveInterval = 60000;

MessageProcessor::MessageProcessor(QObject *parent) :
    QObject(parent), output(stdout), endDayCron(-1), headaid(-1), bid(-1), shouldRun(true)
{
    proc = new QProcess(this);
    proc->setReadChannel(QProcess::StandardOutput);
    QStringList args;
    args << "-C" << "-R" << "-I";

    hourlyCron = QTime::currentTime().hour();

    connect(proc, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(proc, SIGNAL(finished(int)), this, SLOT(rerunTelegram()));
    proc->start(QCoreApplication::arguments()[1], args, QProcess::ReadWrite);

    keepAliveTimer = new QTimer(this);
    connect(keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
    keepAliveTimer->start(keepAliveInterval);

    calculator = new Calculator(this, this);

    database = new Database(&output, this);
    nameDatabase = new NameDatabase(database, this, this);
    group = new Group(database, nameDatabase, this, this);
    subscribe = new Subscribe(database, nameDatabase, this, this);
    stats = new Statistics(database, nameDatabase, this, this);
    help = new Help(this, this);
    sup = new Sup(database, nameDatabase, this, subscribe, this);
    banlist = new BanList(database, nameDatabase, this, this);
    poll = new Poll(database, nameDatabase, this, subscribe, this);
    broadcast = new Broadcast(nameDatabase, this, this);
    tree = new Tree(nameDatabase, this, this);
    protect = new Protect(database, nameDatabase, this, this);
    headAdmin = new HeadAdmin(database, nameDatabase, this, this);

    loadConfig();

    QTimer::singleShot(0, this, SLOT(keepAlive()));
}

MessageProcessor::~MessageProcessor()
{
    shouldRun = false;
    proc->terminate();
}

void MessageProcessor::loadConfig()
{
    QSqlQuery query;
    query.prepare("SELECT headadmin_id, bot_id FROM tf_config");
    database->executeQuery(query);

    if (query.next())
    {
        headaid = query.value(0).toLongLong();
        bid = query.value(1).toLongLong();
    }
}

void MessageProcessor::readData()
{
    while (proc->canReadLine())
    {
        QString str = QString::fromUtf8(proc->readLine());
        nameDatabase->input(str);
        protect->rawInput(str);

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
                    gid = str.mid(chatidx, str.indexOf(' ', chatidx) - chatidx);
                uid = str.mid(useridx, str.indexOf(' ', useridx) - useridx);

                QString identity = gid.isNull() ? uid : gid;

                output << "Got From " << identity << ": " << cmd << ' ' << endl << flush;

                qint64 gidnum = gid.mid(5).toLongLong();
                qint64 uidnum = uid.mid(5).toLongLong();

                group->input(gid, uid, cmd);

                bool inpm = gid.isEmpty();

                if (inpm)
                {
                    qint64 newgid = group->groupForUser(uidnum);
                    if (newgid != -1)
                    {
                        gidnum = newgid;
                        gid = "chat#" + QString::number(newgid);
                    }
                }

                processAs(gid, uid, cmd, inpm);
                uidnum = uid.mid(5).toLongLong();

                if (!banlist->bannedUsers()[gidnum].contains(uidnum) || inpm)
                {
                    help->input(gid, uid, cmd, inpm);
                }

                if (inpm)
                {
                    calculator->input(gid, uid, cmd, inpm);
                }

                if (!banlist->bannedUsers()[gidnum].contains(uidnum))
                {
                    sup->input(gid, uid, cmd, inpm);
                    poll->input(gid, uid, cmd, inpm);
                    tree->input(gid, uid, cmd, inpm);
                    subscribe->input(gid, uid, cmd, inpm);
                }

                stats->input(gid, uid, cmd, inpm);
                protect->input(gid, uid, cmd, inpm);
                banlist->input(gid, uid, cmd, inpm);
                broadcast->input(gid, uid, cmd, inpm);
                headAdmin->input(gid, uid, cmd, inpm);
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
        stats->saveData();
        poll->saveData();
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
            stats->giveStat(gid, "Yesterday", "maxcount", "1-10");
            stats->giveStat(gid, "Yesterday", "maxlength", "1-10");
            stats->giveStat(gid, "Yesterday", "maxdensity", "1-10");
            stats->giveStat(gid, "Yesterday", "activity");
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

void MessageProcessor::processAs(const QString &gid, QString &uid, QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    QByteArray respondTo = inpm ? uid.toUtf8() : gid.toUtf8();

    if (nameDatabase->groups().keys().contains(gidnum)
        && (nameDatabase->groups()[gidnum].first == uidnum || uidnum == headaid)
        && str.startsWith("!as"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        if (args.size() > 2)
        {
            bool ok;
            qint64 targetuid = args[1].toLongLong(&ok);

            if (ok && nameDatabase->userList(gidnum).keys().contains(targetuid))
            {
                int idx = str.indexOf(' ', str.indexOf(args[1]));
                while (idx < str.size() && str[idx] == ' ')
                    ++idx;

                str = str.mid(idx).trimmed();
                uid = "user#" + QString::number(targetuid);

                sendCommand("msg " + respondTo + " \"As command executed successfully!\"");
            }
        }
    }
}

void MessageProcessor::rerunTelegram()
{
    if (shouldRun)
    {
        qDebug() << "here rerunning bot...";
        proc->setReadChannel(QProcess::StandardOutput);
        QStringList args;
        args << "-C" << "-R" << "-I";
        proc->start(QCoreApplication::arguments()[1], args, QProcess::ReadWrite);
        keepAlive();
    }
}
