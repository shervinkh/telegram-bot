#include "help.h"
#include "messageprocessor.h"
#include <QtCore>

Help::Help(MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), messageProcessor(msgproc)
{
}

void Help::input(const QString &gid, const QString &uid, const QString &msg)
{
    if (msg.startsWith("!help"))
    {
        QString message;

        int idx = msg.indexOf(' ');
        QString cmd = msg.mid(idx + 1).remove(' ').trimmed();
        QString identity = gid.isNull() ? uid : gid;

        if (cmd == "calc")
            message = QString("This module evaluates a mathematical expression.\\n"
                              "e.g. !calc 2*(5**3)*atan2(1, 2)*sin(pi/4)");
        else if (cmd == "help")
            message = QString("This module describes other modules and shows examples for using them\\n"
                              "e.g. !help calc\\n"
                              "e.g. !help");
        else if (cmd == "stat")
            message = QString("This module gathers statistical data from the groups I monitor\\n"
                              "Note: Usable only by group's admin\\n"
                              "Usage: !stat date operation limit\\n"
                              "e.g. !stat Today summary\\n"
                              "e.g. !stat Yesterday maxlength 1-10\\n"
                              "e.g. !stat 2014/01/01 maxcount 2");
        else if (cmd == "sup")
            message = QString("This module can keep a bunch of important things happening on the "
                              "groups I monitor so that newcomers would get a clue of what's happening.\\n"
                              "Note: Currently 'sup entries with multiple lines aren't supported.\\n"
                              "e.g. !sup add message\\n"
                              "e.g. !sup delete 2-5\\n"
                              "e.g. !sup remove 5\\n"
                              "e.g. !sup");
        else if (cmd == "banlist")
            message = QString("This module can ban some users from using my features in the groups I monitor.\\n"
                              "Note: Usable only by group's admin\\n"
                              "e.g. !banlist add 42933206\\n"
                              "e.g. !banlist delete 42933206\\n"
                              "e.g. !banlist");
        else
            message = QString("Current modules are: calc stat help sup banlist\\n"
                              "Enter \\\"!help the_module\\\" (e.g. !help calc) for more help.");

        messageProcessor->sendCommand("msg " + identity.toLatin1() + " \"" + message.toLatin1() + '"');
    }
}
