#include "broadcast.h"
#include "messageprocessor.h"
#include "namedatabase.h"
#include "permission.h"
#include <QtCore>

Broadcast::Broadcast(NameDatabase *namedb, MessageProcessor *msgproc, Permission *perm, QObject *parent) :
    QObject(parent), nameDatabase(namedb), messageProcessor(msgproc), permission(perm)
{
}

void Broadcast::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!broadcast"))
    {
        int perm = permission->getPermission(gidnum, "broadcast", "use", isAdmin, inpm);

        if (perm == 1)
        {
            int idx = str.indexOf(' ');

            if (idx != -1)
            {
                while (idx < str.size() && str[idx] == ' ')
                    ++idx;

                if (idx < str.size())
                {
                    QString msg = str.mid(idx).trimmed();
                    QString totalMsg = QString("Broadcast message from \"%1\" group:\\n%2")
                                       .arg(nameDatabase->groups()[gidnum].second)
                                       .arg(msg);

                    foreach (qint64 uid, nameDatabase->userList(gidnum).keys())
                        messageProcessor->sendMessage("user#" + QString::number(uid), totalMsg);

                    int numMemebers = nameDatabase->userList(gidnum).size();

                    messageProcessor->sendMessage(inpm ? uid : gid,
                                                  QString("Sent broadcast message to %1 group member(s)!")
                                                  .arg(numMemebers));
                }
            }
        }
        else if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }
}
