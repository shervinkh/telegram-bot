#ifndef BFF_H
#define BFF_H

#include <QObject>
#include <QMap>
#include <QPair>

class NameDatabase;
class Database;
class MessageProcessor;
class Score;

class BFF : public QObject
{
    Q_OBJECT
private:
    NameDatabase *nameDatabase;
    Database *database;
    MessageProcessor *messageProcessor;
    Score *score;

    enum BFFState {Neutral, Happy, SoHappy, Sad, SoSad};

    typedef QMap<QString, int> Config;

    QMap<qint64, Config> conf;

    void loadData();
    void setConf(qint64 gid, const QString &str, int value);
    void checkFillDefaultConfig(qint64 gid);

    typedef QPair<QString, int> ConfigData;

    static QList<ConfigData> defaultConfigs;
    static void loadDefaultConfig();
    static BFFState findStateFromMessage(const QString &str);
    static bool isLoveMessage(const QString &str, bool inpm);
    static bool findBot(const QString &str);
    static QList<QString> happySmileys();
    static QList<QString> sadSmileys();
    static QList<QString> soHappySmileys();
    static QList<QString> soSadSmileys();
    static QList<QString> heartSmileys();
    static QList<QString> loveSmileys();

public:
    explicit BFF(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Score *scor, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm);
    void BFFScoresTrigger(const QString &identity, qint64 gidnum, qint64 uidnum, int diff);
    void BFFGetScoredTrigger(const QString &identity, qint64 gidnum, qint64 uidnum, int diff);
    void groupDeleted(qint64 gid);

public slots:
};

#endif // BFF_H
