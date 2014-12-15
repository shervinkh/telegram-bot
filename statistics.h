#ifndef STATISTICS_H
#define STATISTICS_H

#include <QObject>
#include <QMap>
#include <QSet>

class UserStatsData
{
private:
    qint64 cnt;
    qint64 len;

public:
    UserStatsData() : cnt(0), len(0) {}
    UserStatsData(qint64 _cnt, qint64 _len)
        : cnt(_cnt), len(_len) {}

    const qint64 &count() const {return cnt;}
    const qint64 &length() const {return len;}
    qint64 &count() {return cnt;}
    qint64 &length() {return len;}
};

class Database;
class NameDatabase;
class MessageProcessor;

class Statistics : public QObject
{
    Q_OBJECT
private:
    typedef QMap<qint64, UserStatsData> Users;
    typedef QMap<qint64, Users> Groups;
    typedef QMap<qint64, Groups> Dates;

    typedef QPair<UserStatsData, qint64> DataPair;

    typedef QPair<qint64, QByteArray> DateAndSendee;
    typedef QPair<qint64, DateAndSendee> QueueData;

    static const int GraphDelay;

    Dates data;

    QList<DataPair> tempList;
    QList<QueueData> activityQueue;

    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    void loadData();

    static bool compareByCount(const DataPair &a, const DataPair &b)
    {
        return a.first.count() > b.first.count();
    }

    static bool compareByLength(const DataPair &a, const DataPair &b)
    {
        return a.first.length() > b.first.length();
    }

    static bool compareByDensity(const DataPair &a, const DataPair &b)
    {
        return (static_cast<qreal>(a.first.length()) / a.first.count()) >
               (static_cast<qreal>(b.first.length()) / b.first.count());
    }

public:
    explicit Statistics(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    ~Statistics();
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);
    void giveStat(qint64 gid, const QString &date, const QString &factor, const QString &limit = QString(),
                  qint64 uid = -1);
    void cleanUpBefore(qint64 date);
    void saveData();

public slots:
    void processGraph();
    void processActivityQueue();
};

#endif // STATISTICS_H
