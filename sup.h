#ifndef SUP_H
#define SUP_H

#include <QObject>
#include <QMap>
#include <QList>

class MessageProcessor;
class Database;
class NameDatabase;

class SupEntry
{
private:
    qint64 id;
    QString txt;

public:
    SupEntry() {}
    SupEntry(qint64 _id, const QString &str) : id(_id), txt(str) {}

    qint64 Id() const {return id;}
    QString text() const {return txt;}
};

class Sup : public QObject
{
    Q_OBJECT
private:
    static const int MaxSupPerGroup;

    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;

    typedef QList<SupEntry> SupEntries;

    QMap<qint64, SupEntries> data;

    void loadData();

    bool addEntry(qint64 gid, const QString &str);
    void delEntry(qint64 gid, int id);

public:
    explicit Sup(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent = 0);
    void input(const QString &gid, const QString &, const QString &str);

public slots:

};

#endif // SUP_H
