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
    QTextStream *output;

    void prepareDatabase();

public:
    explicit Database(QTextStream *out, QObject *parent = 0);
    void executeQuery(QSqlQuery &query);
};

#endif // DATABASE_H
