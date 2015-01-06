#include "headadmin.h"
#include "database.h"
#include "namedatabase.h"
#include "messageprocessor.h"
#include <QtCore>
#include <QSqlQuery>

HeadAdmin::HeadAdmin(Database *db, NameDatabase *namedb, MessageProcessor *msgproc, QObject *parent)
    : QObject(parent), database(db), nameDatabase(namedb), messageProcessor(msgproc)
{

}

void HeadAdmin::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (uidnum == messageProcessor->headAdminId() && str.startsWith("!headadmin"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);
        QString message;

        if (args.size() > 4 && args[1].toLower().startsWith("mon"))
        {
            bool ok1, ok2;
            qint64 curgid, curuid;

            if (args[2].toLower().startsWith("here"))
            {
                curgid = gidnum;
                ok1 = true;
            }
            else
                curgid = args[2].toLongLong(&ok1);

            curuid = args[3].toLongLong(&ok2);

            int idx = str.indexOf(' ', str.indexOf(args[3]));
            while (str[idx] == ' ')
                ++idx;

            QString name = str.mid(idx).trimmed();

            if (ok1 && ok2 && !nameDatabase->groups().keys().contains(curgid))
            {
                addMonitor(curgid, curuid, name);
                message = QString("Added group#%1 with user#%2 as bot's admin and \"%3\" as name to the monitoring database!")
                          .arg(curgid).arg(curuid).arg(name);
            }
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("unmon"))
        {
            bool ok;
            qint64 curgid;

            if (args[2].toLower().startsWith("here"))
            {
                curgid = gidnum;
                ok = true;
            }
            else
                curgid = args[2].toLongLong(&ok);

            if (ok && nameDatabase->groups().keys().contains(curgid))
            {
                delMonitor(curgid);
                database->deleteGroup(curgid);
                message = QString("Removed group#%1 from monitoring database!").arg(curgid);
            }
        }
        else if (args.size() > 3 && args[1].toLower().startsWith("admin"))
        {
            bool ok1, ok2;
            qint64 curgid = args[2].toLongLong(&ok1);
            qint64 curuid = args[3].toLongLong(&ok2);

            if (ok1 && ok2 && nameDatabase->groups().keys().contains(curgid))
            {
                changeAdmin(curgid, curuid);
                message = QString("Changed group#%1's admin to user#%2!").arg(curgid).arg(curuid);
            }
        }
        else if (args.size() > 3 && args[1].toLower().startsWith("name"))
        {
            bool ok;
            qint64 curgid = args[2].toLongLong(&ok);

            if (ok && nameDatabase->groups().keys().contains(curgid))
            {
                int idx = str.indexOf(' ', str.indexOf(args[2]));
                while (str[idx] == ' ')
                    ++idx;

                QString name = str.mid(idx).trimmed();

                changeName(curgid, name);
                message = QString("Changed group#%1's name to \"%2\"!").arg(curgid).arg(name);
            }
        }
        else if (args.size() > 1 && args[1].toLower().startsWith("list"))
        {
            message = "Monitoring groups:";

            if (nameDatabase->groups().isEmpty())
                message += "\\nNo Group!";
            else
            {
                int idx = 1;
                foreach (qint64 curgid, nameDatabase->groups().keys())
                    message += QString("\\n%1- group#%2 (Admin: %3 - %4) (Name: %5)").arg(idx++)
                               .arg(curgid)
                               .arg(nameDatabase->groups()[curgid].first)
                               .arg(messageProcessor->convertToName(nameDatabase->groups()[curgid].first))
                               .arg(nameDatabase->groups()[curgid].second);
            }
        }
        else
            message = QString("The headadmin is %1(%2).").arg(messageProcessor->headAdminId())
                        .arg(messageProcessor->convertToName(messageProcessor->headAdminId()));

        if (!message.isEmpty())
            messageProcessor->sendCommand("msg " + (inpm ? uid.toUtf8() : gid.toUtf8()) + " \"" +
                                          message.replace('"', "\\\"").toUtf8() + '"');
    }
}

void HeadAdmin::addMonitor(qint64 gid, qint64 aid, const QString &str)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO tf_groups (gid, admin_id, name) VALUES (:gid, :aid, :name)");
    query.bindValue(":gid", gid);
    query.bindValue(":aid", aid);
    query.bindValue(":name", str);
    database->executeQuery(query);
    nameDatabase->loadData();
}

void HeadAdmin::delMonitor(qint64 gid)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_groups WHERE gid=:gid");
    query.bindValue(":gid", gid);
    database->executeQuery(query);
    nameDatabase->loadData();
}

void HeadAdmin::changeAdmin(qint64 gid, qint64 aid)
{
    QSqlQuery query;
    query.prepare("UPDATE tf_groups SET admin_id=:aid WHERE gid=:gid");
    query.bindValue(":aid", aid);
    query.bindValue(":gid", gid);
    database->executeQuery(query);
    nameDatabase->loadData();
}

void HeadAdmin::changeName(qint64 gid, const QString &str)
{
    QSqlQuery query;
    query.prepare("UPDATE tf_groups SET name=:name WHERE gid=:gid");
    query.bindValue(":name", str);
    query.bindValue(":gid", gid);
    database->executeQuery(query);
    nameDatabase->loadData();
}
