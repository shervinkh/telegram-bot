#ifndef TALK_H
#define TALK_H

#include <QObject>
#include <QMap>

class NameDatabase;
class Database;
class MessageProcessor;
class Permission;
class Nickname;

class TalkRecord
{
private:
    QString txt;
    QString rct;

public:
    TalkRecord() {}
    TalkRecord(const QString &text, const QString &react) : txt(text), rct(react) {}

    QString &text() {return txt;}
    QString &react() {return rct;}
    const QString &text() const {return txt;}
    const QString &react() const {return rct;}
};

class Talk : public QObject
{
    Q_OBJECT
private:
    static const int MaxSimilarContainLength;
    static const qreal RecognitionThreshold;
    static const int MaxTalksPerGroup;

    NameDatabase *nameDatabase;
    Database *database;
    MessageProcessor *messageProcessor;
    Permission *permission;
    Nickname *nickname;

    typedef QMap<qint64, TalkRecord> Records;
    typedef QMap<qint64, Records> Groups;

    Groups data;

    void loadData();
    qint64 createTalk(qint64 gid, const QString &txt);
    void editText(qint64 gid, qint64 id, const QString &txt);
    void editReact(qint64 gid, qint64 id, const QString &react);
    void delTalk(qint64 gid, qint64 id);
    QString findReact(qint64 gid, qint64 sender, const QString &text);
    QString processMessage(qint64 gid, qint64 senderid, const QString &text);
    QString randomName(qint64 gid, qint64 uid);

    QString filterString(const QString &str);
    int similarContain(QString str2, QString str1);

public:
    explicit Talk(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Permission *perm,
                  Nickname *nick, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);

public slots:
};

#endif // TALK_H
