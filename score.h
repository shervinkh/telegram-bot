#ifndef SCORE_H
#define SCORE_H

#include <QObject>
#include <QPair>
#include <QMap>

class NameDatabase;
class Database;
class MessageProcessor;
class Permission;
class Nickname;

class MessageData
{
private:
    qint64 u;
    QList<qint64> lu;

public:
    MessageData() {}
    MessageData(qint64 uid) : u(uid) {}

    qint64 uid() const {return u;}
    QList<qint64> &likedUsers() {return lu;}
};

class Score : public QObject
{
    Q_OBJECT
private:
    static const int MaxSimilarContainLength;
    static const int MessagesToKeep;
    static const int DaysToKeep;
    static const qreal RecognitionThreshold;
    static const qreal EPS;

    NameDatabase *nameDatabase;
    Database *database;
    MessageProcessor *messageProcessor;
    Permission *permission;
    Nickname *nick;

    typedef QList<MessageData> MessageDatas;
    QMap<qint64, MessageDatas> msgs;

    typedef QMap<qint64, qint64> UserScores;
    QMap<qint64, UserScores> scoreData;

    typedef QPair<qint64, qint64> DataPair;
    QList<DataPair> tempList;

    QString filterString(const QString &str);
    int similarContain(QString str2, QString str1);

    int isLikeOrDislike(QString &str);

    void loadData();

public:
    explicit Score(NameDatabase *namedb, Database *db, MessageProcessor *msgproc,
                   Permission *perm, Nickname *nck, QObject *parent = 0);
    void saveData();
    void input(const QString &gid, const QString &uid, QString str, bool inpm, bool isAdmin);
    void dailyCron();

public slots:
};

#endif // SCORE_H
