#include "database.h"
#include <QtCore>
#include <QtSql>

Database::Database(QTextStream *out, QObject *parent) :
    QObject(parent), output(out)
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("tf.db");

    if (!database.open())
        qFatal("Could not open the database!");

    prepareDatabase();
}

void Database::executeQuery(QSqlQuery &query)
{
    if (!query.exec())
        (*output) << "Sql Error: " << query.executedQuery() << endl
                  << query.lastError().text() << endl << flush;
}

void Database::prepareDatabase()
{
    QSqlQuery query;

    query.prepare("CREATE TABLE IF NOT EXISTS tf_userstat (gid INTEGER, uid INTEGER, date INTEGER, "
                  "count INTEGER, length INTEGER, PRIMARY KEY (gid, uid, date))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_groups (gid INTEGER PRIMARY KEY)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_sup (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "gid INTEGER, text TEXT)");
    executeQuery(query);
}
