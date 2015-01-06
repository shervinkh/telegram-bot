#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QTextStream>
#include <QMap>

class QProcess;
class QTimer;
class Calculator;
class NameDatabase;
class Statistics;
class Database;
class Help;
class Sup;
class BanList;
class Poll;
class Broadcast;
class Tree;
class Subscribe;
class Group;
class Protect;
class HeadAdmin;

class MessageProcessor : public QObject
{
    Q_OBJECT
private:
    static const qint64 keepAliveInterval;

    QTextStream output;
    QProcess *proc;

    QTimer *keepAliveTimer;

    Calculator *calculator;
    Database *database;
    NameDatabase *nameDatabase;
    Statistics *stats;
    Help *help;
    Sup *sup;
    BanList *banlist;
    Poll *poll;
    Broadcast *broadcast;
    Tree *tree;
    Subscribe *subscribe;
    Group *group;
    Protect *protect;
    HeadAdmin *headAdmin;

    qint64 endDayCron;
    qint64 hourlyCron;

    qint64 headaid;
    qint64 bid;

    bool shouldRun;

    void processAs(const QString &gid, QString &uid, QString &str, bool inpm);
    void loadConfig();

public:
    explicit MessageProcessor(QObject *parent = 0);
    ~MessageProcessor();
    void sendCommand(const QByteArray &str);

    qint64 headAdminId() const {return headaid;}
    qint64 botId() const {return bid;}

    static qint64 processDate(const QString &str);
    QString convertToName(qint64 id);

public slots:
    void readData();
    void keepAlive();
    void rerunTelegram();
};

#endif // MESSAGEPROCESSOR_H
