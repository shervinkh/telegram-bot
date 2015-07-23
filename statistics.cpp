#include "statistics.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include "permission.h"
#include <QSqlQuery>
#include <QVariant>
#include <QtCore>

const int Statistics::GraphDelay = 5000;

Statistics::Statistics(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), permission(perm)
{
    activityQueueTimer = new QTimer(this);
    activityQueueTimer->setInterval(GraphDelay);
    connect(activityQueueTimer, SIGNAL(timeout()), this, SLOT(processActivityQueue()));

    loadData();
}

void Statistics::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT gid, uid, date, count, length, online FROM tf_userstat");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        qint64 uid = query.value(1).toLongLong();
        qint64 date = query.value(2).toLongLong();
        qint64 cnt = query.value(3).toLongLong();
        qint64 len = query.value(4).toLongLong();
        qint64 onl = query.value(5).toLongLong();

        data[date][gid][uid] = UserStatsData(cnt, len, onl);
    }
}

void Statistics::groupDeleted(qint64 gid)
{
    data.remove(gid);
}

void Statistics::saveData()
{
    finalizeOnlineTimes();
    QMapIterator<qint64, Groups> datesIter(data);
    while (datesIter.hasNext())
    {
        datesIter.next();
        qint64 date = datesIter.key();

        QMapIterator<qint64, Users> groupsIter(datesIter.value());
        while (groupsIter.hasNext())
        {
            groupsIter.next();
            qint64 gid = groupsIter.key();

            QMapIterator<qint64, UserStatsData> statsIter(groupsIter.value());
            while (statsIter.hasNext())
            {
                statsIter.next();

                QSqlQuery query;
                query.prepare("INSERT OR IGNORE INTO tf_userstat (gid, uid, date) "
                              "VALUES(:gid, :uid, :date)");
                query.bindValue(":gid", gid);
                query.bindValue(":uid", statsIter.key());
                query.bindValue(":date", date);
                database->executeQuery(query);

                query.prepare("UPDATE tf_userstat SET count=:cnt, length=:len, online=:onl WHERE gid=:gid AND "
                              "uid=:uid AND date=:date");
                query.bindValue(":cnt", statsIter.value().count());
                query.bindValue(":len", statsIter.value().length());
                query.bindValue(":onl", statsIter.value().online());
                query.bindValue(":gid", gid);
                query.bindValue(":uid", statsIter.key());
                query.bindValue(":date", date);
                database->executeQuery(query);
            }
        }
    }
}

void Statistics::rawInput(const QString &str)
{
    int idxOfUser = str.indexOf("user#");
    if (idxOfUser != -1)
    {
        qint64 uid = str.mid(idxOfUser + 5, str.indexOf('\e', idxOfUser + 5) - (idxOfUser + 5)).toLongLong();
        int idxOfMessage = str.indexOf(' ', idxOfUser);

        if (idxOfMessage != -1)
        {
            QString message = str.mid(idxOfMessage + 1);
            if (message.startsWith("online"))
            {
                if (onlineTime.contains(uid))
                {
                    qint64 diffTime = QDateTime::currentMSecsSinceEpoch() / 1000 - onlineTime[uid];
                    data[QDate::currentDate().toJulianDay()][0][uid].online() += diffTime;
                }
                onlineTime[uid] = QDateTime::currentMSecsSinceEpoch() / 1000;
            }
            else if (message.startsWith("offline") && onlineTime.contains(uid))
            {
                qint64 diffTime = QDateTime::currentMSecsSinceEpoch() / 1000 - onlineTime[uid];
                data[QDate::currentDate().toJulianDay()][0][uid].online() += diffTime;
                onlineTime.remove(uid);
            }
        }
    }
}

void Statistics::finalizeOnlineTimes()
{
    qint64 curTime = QDateTime::currentMSecsSinceEpoch() / 1000;

    foreach (qint64 uid, onlineTime.keys())
    {
        qint64 diffTime = curTime - onlineTime[uid];
        data[QDate::currentDate().toJulianDay()][0][uid].online() += diffTime;
        onlineTime[uid] = curTime;
    }
}

void Statistics::input1(const QString &gid, const QString &uid, const QString &str)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 usernum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
    {
        ++data[QDate::currentDate().toJulianDay()][gidnum][usernum].count();
        data[QDate::currentDate().toJulianDay()][gidnum][usernum].length() += str.length();
        ++data[QDate::currentDate().toJulianDay()][gidnum][0].count();
        data[QDate::currentDate().toJulianDay()][gidnum][0].length() += str.length();
        ++data[QDate::currentDate().toJulianDay()][gidnum][-1 - QTime::currentTime().hour()].count();
    }
}

void Statistics::input2(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 usernum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
    {
        if (str.startsWith("!stat"))
        {
            QStringList args = str.split(' ', QString::SkipEmptyParts);

            if (args.size() > 1 && args[1].toLower().startsWith("online"))
            {
                inpm = inpm || (args.size() > 2 && args[2].toLower().startsWith("pm"));

                int perm = permission->getPermission(gidnum, "stat", "online_list", isAdmin, inpm);

                if (perm == 1)
                {
                    QString message = "List of online users: ";
                    QList<qint64> userlist = nameDatabase->userList(gidnum).keys();

                    int idx = 1;
                    foreach (qint64 uuid, onlineTime.keys())
                        if (userlist.contains(uuid))
                            message += QString("\n%1- %2").arg(idx++)
                                       .arg(messageProcessor->convertToName(uuid));

                    messageProcessor->sendMessage(inpm ? uid : gid, message);
                }
                else if (perm == 2)
                    permission->sendRequest(gid, uid, str, inpm);
            }
            else if (args.size() >= 3)
            {
                inpm = inpm || args.last().startsWith("pm");
                giveStat(gid.mid(5).toLongLong(), args[1], args[2], args.size() > 3 ? args[3] : "",
                         usernum, inpm, isAdmin);
            }
        }
    }
}

void Statistics::giveStat(qint64 gid, const QString &date, const QString &factor, const QString &limit,
                          qint64 uid, bool inpm, bool isAdmin)
{
    qint64 dateNum = MessageProcessor::processDate(date);
    QString sendee = (uid == -1 || !inpm) ? ("chat#" + QString::number(gid)) : ("user#" + QString::number(uid));

    if (dateNum > 0 && data.contains(dateNum) && data[dateNum].contains(gid))
    {
        QString result;
        int perm = 0;

        if (factor.toLower().startsWith("sum"))
        {
            perm = permission->getPermission(gid, "stat", "summary", isAdmin, inpm);
            if (perm == 1)
            {
                result = QString("Statistics Summary For %1:\\nTotal Posts' Count: %2\\n"
                                 "Total Posts' Length: %3").arg(date)
                        .arg(data[dateNum][gid][0].count()).arg(data[dateNum][gid][0].length());
            }
        }
        else if (factor.toLower().startsWith("maxc") || factor.toLower().startsWith("maxl")
                 || factor.toLower().startsWith("maxd") || factor.toLower().startsWith("maxo"))
        {
            perm = permission->getPermission(gid, "stat", "max_lists", isAdmin, inpm);
            if (perm == 1)
            {
                int type = factor.toLower().startsWith("maxc") ? -2 :
                           (factor.toLower().startsWith("maxl") ? -1 : factor.toLower().startsWith("maxo"));
                int start, end;
                bool ok1, ok2;

                if (limit.toLower().startsWith("all"))
                {
                    start = 1;
                    end = data[dateNum][gid].size();
                    ok1 = ok2 = true;
                }
                else
                {
                    int idx = limit.indexOf('-');

                    if (idx == -1)
                    {
                        start = end = limit.toInt(&ok1);
                        ok2 = true;
                    }
                    else
                    {
                        start = limit.mid(0, idx).toInt(&ok1);
                        end = limit.mid(idx + 1).toInt(&ok2);
                    }
                }

                if (ok1 && ok2 && start > 0 && end > 0 && end >= start)
                {
                    tempList.clear();

                    if (type == 1)
                        finalizeOnlineTimes();

                    QMapIterator<qint64, UserStatsData> usersIter(data[dateNum][(type == 1) ? 0 : gid]);
                    QList<qint64> userlist = nameDatabase->userList(gid).keys();
                    while (usersIter.hasNext())
                    {
                        usersIter.next();

                        if (usersIter.key() > 0 && userlist.contains(usersIter.key()))
                            tempList.append(DataPair(usersIter.value(), usersIter.key()));
                    }

                    start = qMin(start, tempList.length());
                    end = qMin(end, tempList.length());

                    qSort(tempList.begin(), tempList.end(),
                          (type == -2) ? compareByCount : ((type == -1) ? compareByLength : (type ? compareByOnline : compareByDensity)));

                    result = QString("People with most %1 on %2:\n")
                            .arg((type == -2) ? "post count" : ((type == -1) ? "post length" : (type ? "online time" : "post density")))
                            .arg(date);

                    for (int i = start; i <= end; ++i)
                    {

                        QString number;
                        if (type == 1)
                        {
                            qint64 secs = tempList[i - 1].first.online();
                            qint64 mins = secs / 60;
                            secs %= 60;
                            qint64 hrs = mins / 60;
                            mins %= 60;

                            number = QString("%1:%2:%3").arg(hrs, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
                        }
                        else if (type == 0)
                            number = QString::number(static_cast<qreal>(tempList[i - 1].first.length()) / tempList[i - 1].first.count(), 'f', 2);
                        else
                            number = QString::number((type == -2) ? tempList[i - 1].first.count() : tempList[i - 1].first.length());

                        result += QString("%1- %2 (%3)").arg(i).arg(messageProcessor->convertToName(tempList[i - 1].second))
                                .arg(number);

                        if (i != end)
                            result += "\n";
                    }
                }
            }
        }
        else if (factor.toLower().startsWith("act"))
        {
            perm = permission->getPermission(gid, "stat", "activity", isAdmin, inpm);
            if (perm == 1)
            {
                activityQueue.push_back(QueueData(gid, DateAndSendee(dateNum, sendee.toUtf8())));

                if (activityQueue.size() == 1 && !activityQueueTimer->isActive())
                    processActivityQueue();
            }
        }

        if (!result.isEmpty() && dateNum == QDate::currentDate().toJulianDay())
            result += QString("\nSo far");

        messageProcessor->sendMessage(sendee, result);

        if (perm == 2)
        {
            QString newgid = "chat#" + QString::number(gid);
            QString newuid = "user#" + QString::number(uid);
            QString cmd = QString("!stat %1 %2 %3").arg(date).arg(factor).arg(limit);
            permission->sendRequest(newgid, newuid, cmd, inpm);
        }
    }
}

void Statistics::cleanUpBefore(qint64 date)
{
    foreach (qint64 d, data.keys())
        if (d < date)
            data.remove(d);

    QSqlQuery query;
    query.prepare("DELETE FROM tf_userstat WHERE date < :date");
    query.bindValue(":date", date);
    database->executeQuery(query);
}

void Statistics::processGraph()
{
    QProcess *proc = qobject_cast<QProcess *>(sender());

    if (proc->exitCode() == 0)
        messageProcessor->sendCommand("send_photo " + activityQueue.front().second.second + " stat.png");

    proc->deleteLater();
    activityQueue.pop_front();
    activityQueueTimer->start();
}

void Statistics::processActivityQueue()
{
    if (activityQueue.isEmpty())
    {
        activityQueueTimer->stop();
        return;
    }

    qint64 gid = activityQueue.front().first;
    qint64 dateNum = activityQueue.front().second.first;

    QFile file("stat.data");

    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream TS(&file);
        TS.setCodec(QTextCodec::codecForName("UTF-8"));

        for (int i = 0; i < 24; ++i)
            TS << i << '\t' << data[dateNum][gid][-1 - i].count() << endl;

        QProcess *gnuplot = new QProcess(this);
        connect(gnuplot, SIGNAL(finished(int)), this, SLOT(processGraph()));

        QStringList args;
        args << "plot_script";

        gnuplot->start("gnuplot", args);
    }
    else
    {
        activityQueue.pop_front();
        processActivityQueue();
    }
}
