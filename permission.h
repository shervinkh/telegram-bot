#ifndef PERMISSION_H
#define PERMISSION_H

#include <QObject>
#include <QMap>
#include <QPair>
#include "messageprocessor.h"

class Database;
class NameDatabase;
class MessageProcessor;
class Request;

class PermissionData
{
private:
    int acc;
    int pmAcc;

public:
    PermissionData() : acc(All), pmAcc(All) {}
    PermissionData(int a, int pa) : acc(a), pmAcc(pa) {}

    int access() const {return acc;}
    int pmAccess() const {return pmAcc;}
    int &access() {return acc;}
    int &pmAccess() {return pmAcc;}
};

class PermissionId
{
private:
    QString mod;
    QString op;
    int acc;
    int pmAcc;

public:
    PermissionId() {}
    PermissionId(const QString &modu, const QString &oper, int ac, int pm_acc)
        : mod(modu), op(oper), acc(ac), pmAcc(pm_acc) {}

    QString module() const {return mod;}
    QString operation() const {return op;}
    int access() const {return acc;}
    int pmAccess() const {return pmAcc;}
};

class Permission : public QObject
{
    Q_OBJECT
private:

    static QList<PermissionId> defaultPermissions;
    static bool isInit;

    Database *database;
    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Request *request;

    typedef QMap<QString, PermissionData> ModulePermissions;
    typedef QMap<QString, ModulePermissions> GroupPermissions;
    typedef QMap<qint64, GroupPermissions> Permissions;
    Permissions data;

    void loadData();
    static void initDefaults();

    int fromName(QString str);
    QString fromValue(int val);
    void fillDefaultPermissions(qint64 gid);
    void editPermission(qint64 gid, const QString &mod, const QString &op, int ac, int pmac);

public:
    explicit Permission(Database *db, NameDatabase *namedb, MessageProcessor *msgproc,
                        Request *req, QObject *parent = 0);
    void checkPermissions(qint64 gid);
    int getPermission(qint64 gid, const QString &mod, const QString &op, bool isAdmin, bool inpm);
    void input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin);
    void sendRequest(const QString &gid, const QString &uid, const QString &cmd, bool inpm);

public slots:
};

#endif // PERMISSION_H
