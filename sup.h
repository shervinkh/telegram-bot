#ifndef SUP_H
#define SUP_H

#include <QObject>
#include <QMap>
#include <QList>

class MessageProcessor;
class Database;
class NameDatabase;
class Subscribe;
class Permission;

class SupEntry
{
private:
    qint64 id;
    QString txt;

public:
    SupEntry() {}
    SupEntry(qint64 _id, const QString &str) : id(_id), txt(str) {}

    qint64 Id() const {return id;}
    QString &text() {return txt;}
};

class Sup : public QObject
{
    Q_OBJECT
private:
    static const int MaxSupPerGroup;
    static const int DistanceBetweenSup;

    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;
    Subscribe *subscribe;

    QMap<qint64, qint64> lastSup;

    typedef QList<SupEntry> SupEntries;

    QMap<qint64, SupEntries> data;

    void loadData();

    bool addEntry(qint64 gid, const QString &str);
    void delEntry(qint64 gid, int id);
    void editEntry(qint64 gid, int id, const QString &str);
    void freshSup();

public:
    explicit Sup(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm,
                 Subscribe *sub, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);

public slots:

};

#endif // SUP_H
