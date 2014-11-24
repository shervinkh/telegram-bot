#include "statistics.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QSqlQuery>
#include <QVariant>
#include <QtCore>

Statistics::Statistics(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{
    loadData();
}

Statistics::~Statistics()
{
    saveData();
}

void Statistics::loadData()
{
    data.clear();

    QSqlQuery query;

    query.prepare("SELECT gid, uid, date, count, length FROM tf_userstat");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        qint64 uid = query.value(1).toLongLong();
        qint64 date = query.value(2).toLongLong();
        qint64 cnt = query.value(3).toLongLong();
        qint64 len = query.value(4).toLongLong();

        data[date][gid][uid] = UserStatsData(cnt, len);
    }
}

void Statistics::saveData()
{
    QSqlDatabase::database().transaction();

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

                query.prepare("UPDATE tf_userstat SET count=:cnt, length=:len WHERE gid=:gid AND "
                              "uid=:uid AND date=:date");
                query.bindValue(":cnt", statsIter.value().count());
                query.bindValue(":len", statsIter.value().length());
                query.bindValue(":gid", gid);
                query.bindValue(":uid", statsIter.key());
                query.bindValue(":date", date);
                database->executeQuery(query);
            }
        }
    }

    QSqlDatabase::database().commit();
}

void Statistics::input(const QString &gid, const QString &uid, const QString &str)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 usernum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
    {
        ++data[QDate::currentDate().toJulianDay()][gidnum][usernum].count();
        data[QDate::currentDate().toJulianDay()][gidnum][usernum].length() += str.length();
        ++data[QDate::currentDate().toJulianDay()][gidnum][0].count();
        data[QDate::currentDate().toJulianDay()][gidnum][0].length() += str.length();

        if (str.startsWith("!stat") && usernum == nameDatabase->groups()[gidnum].first)
        {
            QStringList args = str.split(' ', QString::SkipEmptyParts);
            if (args.size() >= 3)
                giveStat(gid.mid(5).toLongLong(), args[1], args[2], args.size() > 3 ? args[3] : "");
        }
    }
}

void Statistics::giveStat(qint64 gid, const QString &date, const QString &factor, const QString &limit)
{
    qint64 dateNum = MessageProcessor::processDate(date);

    if (dateNum > 0 && data.contains(dateNum) && data[dateNum].contains(gid))
    {
        QString result;

        if (factor.toLower().startsWith("sum"))
            result = QString("Statistics Summary For %1:\\nTotal Posts' Count: %2\\n"
                             "Total Posts' Length: %3").arg(date)
                     .arg(data[dateNum][gid][0].count()).arg(data[dateNum][gid][0].length());
        else if (factor.toLower().startsWith("maxc") || factor.toLower().startsWith("maxl")
                 || factor.toLower().startsWith("maxd"))
        {
            int type = factor.toLower().startsWith("maxc") ? -1 : factor.toLower().startsWith("maxd");
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

                QMapIterator<qint64, UserStatsData> usersIter(data[dateNum][gid]);
                while (usersIter.hasNext())
                {
                    usersIter.next();

                    if (usersIter.key() != 0)
                        tempList.append(DataPair(usersIter.value(), usersIter.key()));
                }

                start = qMin(start, tempList.length());
                end = qMin(end, tempList.length());

                qSort(tempList.begin(), tempList.end(),
                      (type == -1) ? compareByCount : (type ? compareByDensity : compareByLength));

                result = QString("People with most post %1 on %2:\\n")
                        .arg((type == -1) ? "count" : (type ? "density" : "length")).arg(date);

                for (int i = start; i <= end; ++i)
                {
                    QString number;
                    if (type == 1)
                        number = QString::number(static_cast<qreal>(tempList[i - 1].first.length()) / tempList[i - 1].first.count(), 'f', 2);
                    else
                        number = QString::number((type == -1) ? tempList[i - 1].first.count() : tempList[i - 1].first.length());

                    result += QString("%1- %2 (%3)").arg(i).arg(messageProcessor->convertToName(tempList[i - 1].second))
                              .arg(number);

                    if (i != end)
                        result += "\\n";
                }
            }
        }

        if (!result.isNull() && dateNum == QDate::currentDate().toJulianDay())
            result += QString("\\nSo far");

        if (!result.isNull())
            messageProcessor->sendCommand("msg chat#" + QByteArray::number(gid) +
                                          " \"" + result.toUtf8() + '"');
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
