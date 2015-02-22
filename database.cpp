#include "database.h"
#include <QtCore>
#include <QtSql>

Database::Database(QObject *parent) :
    QObject(parent)
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
        qDebug() << "Sql Error: " << query.executedQuery() << endl
                  << query.lastError().text() << endl << flush;
}

void Database::prepareDatabase()
{
    QSqlQuery query;

    query.prepare("CREATE TABLE IF NOT EXISTS tf_userstat (gid INTEGER, uid INTEGER, date INTEGER, "
                  "count INTEGER, length INTEGER, online INTEGER, PRIMARY KEY (gid, uid, date))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_groups (gid INTEGER PRIMARY KEY, admin_id INTEGER, name TEXT)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_banned (gid INTEGER, uid INTEGER, PRIMARY KEY (gid, uid))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_subscribe (gid INTEGER, uid INTEGER, PRIMARY KEY (gid, uid))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_usergroups (uid INTEGER PRIMARY KEY, gid INTEGER)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_sup (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "gid INTEGER, text TEXT)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_polls (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "gid INTEGER, title TEXT, multi_choice BOOLEAN, options TEXT)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_protect (gid INTEGER, "
                  "type INTEGER, value TEXT, PRIMARY KEY (gid, type))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_config (headadmin_id INTEGER, bot_id INTEGER)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_permissions (gid INTEGER, module TEXT, operation TEXT, "
                  "access INTEGER, pm_access INTEGER, PRIMARY KEY (gid, module, operation))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_requests (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "gid INTEGER, uid INTEGER, inpm BOOLEAN, command TEXT)");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_leave_protect (gid INTEGER, uid INTEGER, "
                  "PRIMARY KEY (gid, uid))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_scores (gid INTEGER, uid INTEGER, score INTEGER, "
                  "PRIMARY KEY (gid, uid))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_nicks (gid INTEGER, uid INTEGER, nick TEXT, "
                  "PRIMARY KEY (gid, uid))");
    executeQuery(query);

    query.prepare("CREATE TABLE IF NOT EXISTS tf_talk (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "gid INTEGER, text TEXT, react TEXT)");
    executeQuery(query);
}

void Database::deleteGroup(qint64 gid)
{
    QSqlQuery query;

    query.prepare("DELETE FROM tf_banned WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_userstat WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_subscribe WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_usergroups WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_sup WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_polls WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_protect WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_permissions WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_requests WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_leave_protect WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_scores WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_nicks WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);

    query.prepare("DELETE FROM tf_talk WHERE gid=:gid");
    query.bindValue(":gid", gid);
    executeQuery(query);
}
