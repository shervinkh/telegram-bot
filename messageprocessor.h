#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
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
class Permission;
class Request;
class Score;
class Nickname;

enum {Admin = 0, RequestAdmin, NA, All, Disabled};

class MessageProcessor : public QObject
{
    Q_OBJECT
private:
    static const int MessageLimit;
    static const int MessagesPerBurst;
    static const int BurstDelay;
    static const qint64 keepAliveInterval;

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
    Permission *permission;
    Request *request;
    Score *score;
    Nickname *nick;

    qint64 hourlyCron;

    qint64 headaid;
    qint64 bid;

    bool shouldRun;

    QString cmd, uid, gid;
    bool cmdContinue;

    QList<QByteArray> cmdQueue;
    QTimer *cmdQueueTimer;

    void processAs(const QString &gid, QString &uid, QString &str, bool inpm, bool isAdmin);
    void loadConfig();
    void saveData();

public:
    explicit MessageProcessor(QObject *parent = 0);
    ~MessageProcessor();
    void sendCommand(const QByteArray &str);
    void sendMessage(const QString &identity, QString message);
    void processCommand(const QString &gid, QString uid, QString cmd, bool inpm, bool isAdmin);
    qint64 headAdminId() const {return headaid;}
    qint64 botId() const {return bid;}

    static qint64 processDate(const QString &str);
    QString convertToName(qint64 id);

public slots:
    void readData();
    void keepAlive();
    void runTelegram();
    void processQueue();
};

#endif // MESSAGEPROCESSOR_H
