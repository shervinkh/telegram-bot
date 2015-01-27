#include "messageprocessor.h"
#include "calculator.h"
#include "namedatabase.h"
#include "statistics.h"
#include "permission.h"
#include "broadcast.h"
#include "subscribe.h"
#include "headadmin.h"
#include "database.h"
#include "banlist.h"
#include "protect.h"
#include "request.h"
#include "group.h"
#include "help.h"
#include "poll.h"
#include "tree.h"
#include "sup.h"
#include <QtCore>
#include <QSqlQuery>

const int MessageProcessor::MessageLimit = 4000;
const qint64 MessageProcessor::keepAliveInterval = 60000;

MessageProcessor::MessageProcessor(QObject *parent) :
    QObject(parent), endDayCron(-1), headaid(-1), bid(-1), shouldRun(true), cmdContinue(false)
{
    proc = new QProcess(this);
    hourlyCron = QTime::currentTime().hour();

    connect(proc, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(proc, SIGNAL(finished(int)), this, SLOT(runTelegram()));
    runTelegram();

    keepAliveTimer = new QTimer(this);
    connect(keepAliveTimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
    keepAliveTimer->start(keepAliveInterval);

    database = new Database(this);
    nameDatabase = new NameDatabase(database, this, this);
    request = new Request(database, nameDatabase, this, this);
    permission = new Permission(database, nameDatabase, this, request, this);

    calculator = new Calculator(nameDatabase, this, permission, this);
    group = new Group(database, nameDatabase, this, this);
    subscribe = new Subscribe(database, nameDatabase, this, permission, this);
    stats = new Statistics(database, nameDatabase, this, permission, this);
    help = new Help(nameDatabase, this, permission, this);
    sup = new Sup(database, nameDatabase, this, permission, subscribe, this);
    banlist = new BanList(database, nameDatabase, this, permission, this);
    poll = new Poll(database, nameDatabase, this, permission, subscribe, this);
    broadcast = new Broadcast(nameDatabase, this, permission, this);
    tree = new Tree(nameDatabase, this, permission, this);
    protect = new Protect(database, nameDatabase, this, this);
    headAdmin = new HeadAdmin(database, nameDatabase, this, this);

    request->setSubscribe(subscribe);

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

        if (cmdContinue)
            cmd += str;
        else
        {
            cmd.clear();
            gid.clear();
            uid.clear();

            int idx = qMax(str.indexOf("»»»"), str.indexOf(">>>")) + 3;

            if (idx > 2)
            {
                cmd = str.mid(idx + 1);

                if (cmd.startsWith("[fwd from "))
                    cmd = cmd.mid(cmd.indexOf(']') + 2);

                int chatidx = str.indexOf("chat#");
                int useridx = str.indexOf("user#");

                if (useridx == -1)
                    continue;

                if (chatidx != -1 && chatidx < useridx)
                    gid = str.mid(chatidx, str.indexOf('\e', chatidx) - chatidx);
                uid = str.mid(useridx, str.indexOf('\e', useridx) - useridx);
            }
        }

        if (!cmd.isEmpty())
        {
            int endidx = cmd.indexOf('\e');
            if (endidx == -1)
                cmdContinue = true;
            else
            {
                cmd = cmd.mid(0, endidx);
                cmdContinue = false;
            }

            if (!cmdContinue)
            {
                QString identity = gid.isNull() ? uid : gid;
                qDebug() << "Got From " << identity << ':' << cmd;

                qint64 uidnum = uid.mid(5).toLongLong();
                qint64 gidnum = gid.mid(5).toLongLong();

                bool inpm = gid.isEmpty();
                if (!inpm)
                    stats->input1(gid, uid, cmd);

                group->input(gid, uid, cmd);
                if (inpm)
                {
                    qint64 newgid = group->groupForUser(uidnum);

                    if (newgid != -1)
                    {
                        gidnum = newgid;
                        gid = "chat#" + QString::number(newgid);
                    }
                    else
                        sendCommand("msg " + uid.toUtf8() + " \"Note: In order to use group-related "
                                    "functions in PM, use \\\"!group set group_id\\\" first. "
                                    "You can see your groups' id via \\\"!group\\\".\"");
                }

                bool isAdmin = (uidnum == headaid);
                if (nameDatabase->groups().contains(gidnum))
                    isAdmin = isAdmin || (uidnum == nameDatabase->groups()[gidnum].first);

                processAs(gid, uid, cmd, inpm, isAdmin);

                processCommand(gid, uid, cmd, inpm, isAdmin);
            }
        }
    }
}

void MessageProcessor::processCommand(const QString &gid, QString uid, QString cmd, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (!banlist->bannedUsers()[gidnum].contains(uidnum) || inpm)
    {
        help->input(gid, uid, cmd, inpm, isAdmin);
        calculator->input(gid, uid, cmd, inpm, isAdmin);
    }

    if (!banlist->bannedUsers()[gidnum].contains(uidnum))
    {
        sup->input(gid, uid, cmd, inpm, isAdmin);
        poll->input(gid, uid, cmd, inpm, isAdmin);
        tree->input(gid, uid, cmd, inpm, isAdmin);
        subscribe->input(gid, uid, cmd, inpm, isAdmin);
        stats->input2(gid, uid, cmd, inpm, isAdmin);
        banlist->input(gid, uid, cmd, inpm, isAdmin);
        broadcast->input(gid, uid, cmd, inpm, isAdmin);
    }

    permission->input(gid, uid, cmd, inpm, isAdmin);
    protect->input(gid, uid, cmd, inpm, isAdmin);
    headAdmin->input(gid, uid, cmd, inpm);
    request->input(gid, uid, cmd, inpm, isAdmin);
}

void MessageProcessor::keepAlive()
{
    qDebug() << QString("Keeping alive... (%1)").arg(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"))
           << endl << flush;
    sendCommand("main_session");

    if (QTime::currentTime().hour() != hourlyCron)
    {
        qDebug() << "Running Hourly Cron..." << endl << flush;
        hourlyCron = QTime::currentTime().hour();
        nameDatabase->refreshDatabase();
        stats->saveData();
        poll->saveData();
    }

    if (endDayCron == -1)
        endDayCron = QDate::currentDate().toJulianDay();
    else if (QDate::currentDate().toJulianDay() != endDayCron)
    {
        qDebug() << "Running End-day Cron..." << endl << flush;
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

void MessageProcessor::sendMessage(const QString &identity, QString message)
{
    if (!message.isEmpty())
    {
        message.replace("\n", "\\n");
        message.replace("\"", "\\\"");

        int idx = 0;
        while (!message.isEmpty())
        {
            int newIdx = message.indexOf("\\n", idx);

            if (newIdx == -1)
                newIdx = message.length() - 1;
            else
                ++newIdx;

            if (newIdx > MessageLimit || idx == message.length() - 1)
            {
                if (idx == 0 && newIdx > MessageLimit)
                    message = message.mid(newIdx + 1);
                else
                {
                    sendCommand("msg " + identity.toUtf8() + " \"" +
                                message.mid(0, idx + 1).toUtf8() + '"');
                    message = message.mid(idx + 1);
                }
                idx = 0;
            }
            else
                idx = newIdx;
        }
    }
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

void MessageProcessor::processAs(const QString &gid, QString &uid, QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    QString respondTo = inpm ? uid : gid;

    if (isAdmin && str.startsWith("!as"))
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

                sendMessage(respondTo, "As command executed successfully!");
            }
        }
    }
}

void MessageProcessor::runTelegram()
{
    if (shouldRun)
    {
        qDebug() << "Running bot...";
        proc->setReadChannel(QProcess::StandardOutput);
        QStringList args;
        args << "-W" << "-R" << "-I";
        proc->start(QCoreApplication::arguments()[1], args, QProcess::ReadWrite);
    }
}
