#include "permission.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include "request.h"
#include <QSqlQuery>
#include <QtCore>

bool Permission::isInit = false;
QList<PermissionId> Permission::defaultPermissions;

Permission::Permission(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, Request *req, QObject *parent)
    : QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc), request(req)
{
    if (!isInit)
        initDefaults();

    loadData();
}

void Permission::initDefaults()
{
    defaultPermissions.append(PermissionId("banlist", "add", Admin, Admin));
    defaultPermissions.append(PermissionId("banlist", "delete", Admin, Admin));
    defaultPermissions.append(PermissionId("banlist", "view", All, All));
    defaultPermissions.append(PermissionId("broadcast", "use", Admin, Admin));
    defaultPermissions.append(PermissionId("calc", "use_in_group", All, NA));
    defaultPermissions.append(PermissionId("help", "use_in_group", All, NA));
    defaultPermissions.append(PermissionId("poll", "create", All, All));
    defaultPermissions.append(PermissionId("poll", "create_multi", All, All));
    defaultPermissions.append(PermissionId("poll", "add_option", All, All));
    defaultPermissions.append(PermissionId("poll", "delete_option", All, All));
    defaultPermissions.append(PermissionId("poll", "title_change", All, All));
    defaultPermissions.append(PermissionId("poll", "option_change", All, All));
    defaultPermissions.append(PermissionId("poll", "terminate", All, All));
    defaultPermissions.append(PermissionId("poll", "view_list", All, All));
    defaultPermissions.append(PermissionId("poll", "show", All, All));
    defaultPermissions.append(PermissionId("poll", "result", All, All));
    defaultPermissions.append(PermissionId("poll", "vote", All, All));
    defaultPermissions.append(PermissionId("poll", "unvote", All, All));
    defaultPermissions.append(PermissionId("poll", "who", All, All));
    defaultPermissions.append(PermissionId("stat", "summary", All, All));
    defaultPermissions.append(PermissionId("stat", "max_lists", All, All));
    defaultPermissions.append(PermissionId("stat", "activity", All, All));
    defaultPermissions.append(PermissionId("stat", "online_list", All, All));
    defaultPermissions.append(PermissionId("subscribe", "subscribe", All, All));
    defaultPermissions.append(PermissionId("subscribe", "unsubscribe", All, All));
    defaultPermissions.append(PermissionId("subscribe", "view", All, All));
    defaultPermissions.append(PermissionId("sup", "view", All, All));
    defaultPermissions.append(PermissionId("sup", "add", All, All));
    defaultPermissions.append(PermissionId("sup", "delete", All, All));
    defaultPermissions.append(PermissionId("sup", "edit", All, All));
    defaultPermissions.append(PermissionId("tree", "use", All, All));
    defaultPermissions.append(PermissionId("score", "change_score", All, All));
    defaultPermissions.append(PermissionId("score", "view_score", All, All));
    defaultPermissions.append(PermissionId("nick", "view", All, All));
    defaultPermissions.append(PermissionId("nick", "add", Admin, Admin));
    defaultPermissions.append(PermissionId("nick", "delete", Admin, Admin));
}

void Permission::loadData()
{
    data.clear();

    QSqlQuery query;
    query.prepare("SELECT gid, module, operation, access, pm_access FROM tf_permissions");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        QString module = query.value(1).toString();
        QString operation = query.value(2).toString();
        int access = query.value(3).toInt();
        int pm_access = query.value(4).toInt();

        data[gid][module][operation] = PermissionData(access, pm_access);
    }
}

void Permission::fillDefaultPermissions(qint64 gid)
{
    QSqlDatabase::database().transaction();

    QSqlQuery query;
    foreach (PermissionId perm, defaultPermissions)
    {
        query.prepare("INSERT OR IGNORE INTO tf_permissions (gid, module, operation, access, pm_access) "
                      " VALUES (:gid, :modules, :operation, :access, :pm_access)");
        query.bindValue(":gid", gid);
        query.bindValue(":module", perm.module());
        query.bindValue(":operation", perm.operation());
        query.bindValue(":access", perm.access());
        query.bindValue(":pm_access", perm.pmAccess());

        data[gid][perm.module()][perm.operation()] = PermissionData(perm.access(), perm.pmAccess());

        database->executeQuery(query);
    }

    QSqlDatabase::database().commit();
}

int Permission::getPermission(qint64 gid, const QString &mod, const QString &op, bool isAdmin, bool inpm)
{
    if (!data[gid][mod].contains(op))
    {
        qDebug() << "Filling Default Permissions For: " << gid;
        fillDefaultPermissions(gid);
        qDebug() << "Result: " << data[gid][mod].contains(op);
    }

    int access = inpm ? data[gid][mod][op].pmAccess() : data[gid][mod][op].access();

    if (access == Disabled)
        return 0;

    if (isAdmin)
        return 1;

    if (access == RequestAdmin)
        return 2;
    else if (access == All)
        return 1;

    return 0;
}

void Permission::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!permission"))
    {
        checkPermissions(gidnum);

        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 2 && args[1].toLower().startsWith("show") && data[gidnum].contains(args[2].toLower()))
        {
            QString module = args[2].toLower();

            message = QString("Permissions for module %1:").arg(module);

            foreach (QString key, data[gidnum][module].keys())
                message += QString("\n%1 (Access: %2) (PM-Access: %3)").arg(key)
                        .arg(fromValue(data[gidnum][module][key].access()))
                        .arg(fromValue(data[gidnum][module][key].pmAccess()));
        }
        else if (args.size() > 5 && isAdmin)
        {
            QString module = args[2].toLower();
            QString operation = args[3].toLower();

            if (data[gidnum].contains(module) && data[gidnum][module].contains(operation))
            {
                int ac = fromName(args[4]);
                int pmac = fromName(args[5]);

                bool okac = ac == Admin || ac == All || ac == RequestAdmin || ac == Disabled;
                bool okpmac = pmac == Admin || pmac == All || pmac == RequestAdmin || pmac == Disabled;

                if (okac && okpmac)
                {
                    editPermission(gidnum, module, operation, ac, pmac);
                    message = "Successfully change the permission!";
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);
    }
}

int Permission::fromName(QString str)
{
    if (str.toLower().startsWith("admin"))
        return Admin;
    else if (str.toLower().startsWith("all"))
        return All;
    else if (str.toLower().startsWith("request"))
        return RequestAdmin;
    else if (str.toLower().startsWith("disable"))
        return Disabled;
    else if (str.toLower().startsWith("n/a"))
        return NA;

    return -1;
}

QString Permission::fromValue(int val)
{
    if (val == Admin)
        return "Admin";
    else if (val == All)
        return "All";
    else if (val == RequestAdmin)
        return "Request";
    else if (val == NA)
        return "N/A";
    else if (val == Disabled)
        return "Disabled";

    return "";
}

void Permission::checkPermissions(qint64 gid)
{
    foreach (PermissionId perm, defaultPermissions)
        if (!(data[gid].contains(perm.module()) && data[gid][perm.module()].contains(perm.operation())))
        {
            fillDefaultPermissions(gid);
            return;
        }
}

void Permission::editPermission(qint64 gid, const QString &mod, const QString &op, int ac, int pmac)
{
    if (data[gid][mod][op].pmAccess() == NA)
        pmac = NA;

    QSqlQuery query;
    query.prepare("UPDATE tf_permissions SET access=:access, pm_access=:pm_access "
                  "WHERE gid=:gid AND module=:module AND operation=:operation");
    query.bindValue(":access", ac);
    query.bindValue(":pm_access", pmac);
    query.bindValue(":gid", gid);
    query.bindValue(":module", mod);
    query.bindValue(":operation", op);
    database->executeQuery(query);

    data[gid][mod][op] = PermissionData(ac, pmac);
}

void Permission::sendRequest(const QString &gid, const QString &uid, const QString &cmd, bool inpm)
{
    request->addRequest(gid, uid, cmd, inpm);
}
