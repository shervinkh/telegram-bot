#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>

class QSqlDatabase;
class QSqlQuery;
class QTextStream;

class Database : public QObject
{
    Q_OBJECT
private:
    QSqlDatabase database;

    void prepareDatabase();

public:
    explicit Database(QObject *parent = 0);
    void executeQuery(QSqlQuery &query);
    void deleteGroup(qint64 gid);
};

#endif // DATABASE_H
