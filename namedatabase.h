#ifndef NAMEDATABASE_H
#define NAMEDATABASE_H

#include <QObject>
#include <QMap>
#include <QSet>

class MessageProcessor;
class Database;

class NameDatabase : public QObject
{
    Q_OBJECT
private:
    QMap<qint64, QString> data;
    QSet<qint64> ids;

    MessageProcessor *messageProcessor;
    Database *database;

    QSet<qint64> monitoringGroups;

    qint64 lastIdSeen;

    void getNames();

public:
    explicit NameDatabase(Database *db, MessageProcessor *mp, QObject *parent = 0);
    void input(const QString &str);
    void refreshDatabase();
    const QSet<qint64> &groups() const {return monitoringGroups;}
    const QMap<qint64, QString> &nameDatabase() const {return data;}
};

#endif // NAMEDATABASE_H
