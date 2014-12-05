#include "help.h"
#include "messageprocessor.h"
#include <QtCore>

Help::Help(MessageProcessor *msgproc, QObject *parent) :
    QObject(parent), messageProcessor(msgproc)
{
}

void Help::input(const QString &gid, const QString &uid, const QString &msg, bool inpm)
{
    if (msg.startsWith("!help"))
    {
        QString message;

        int idx = msg.indexOf(' ');
        QString cmd = msg.mid(idx + 1).remove(' ').trimmed();
        QString identity = inpm ? uid : gid;

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
                              "e.g. !stat Today maxdensity all\\n"
                              "e.g. !stat 2014/01/01 maxcount 2");
        else if (cmd == "sup")
            message = QString("This module can keep a bunch of important things happening on the "
                              "groups I monitor so that newcomers would get a clue of what's happening.\\n"
                              "Note: Currently 'sup entries with multiple lines aren't supported.\\n"
                              "e.g. !sup add message\\n"
                              "e.g. !sup delete 2-5\\n"
                              "e.g. !sup remove 5\\n"
                              "e.g. !sup pm\\n"
                              "e.g. !sup");
        else if (cmd == "banlist")
            message = QString("This module can ban some users from using my features in the groups I monitor.\\n"
                              "Note: Usable only by group's admin\\n"
                              "e.g. !banlist add 42933206\\n"
                              "e.g. !banlist delete 42933206\\n"
                              "e.g. !banlist");
        else if (cmd == "poll")
            message = QString("This module can be used to create polls. (Single-Choice and Multi-Choice)\\n"
                              "Single-Choice means each user can vote for one option. Multi-Choice means each "
                              "user can vote for any number of options.\\n"
                              "Note: Currently title and options with multiple lines aren't supported\\n"
                              "e.g. !poll create Are you OK? (Creates Single-Choice poll)\\n"
                              "e.g. !poll create_multi Where are you? (Creates Multi-Choice poll)\\n"
                              "e.g. !poll add_option poll_id option\\n"
                              "e.g. !poll delete_option poll_id option_number\\n"
                              "e.g. !poll terminate poll_id (Deletes a poll)\\n"
                              "e.g. !poll list (pm)\\n"
                              "e.g. !poll vote poll_id option_number\\n"
                              "e.g. !poll unvote poll_id (option_number) (option_number only in Multi-Choice polls)\\n"
                              "e.g. !poll show poll_id (pm)\\n"
                              "e.g. !poll result poll_id (pm)\\n"
                              "e.g. !poll who poll_id option_number (pm) (People voted on an option)");
        else if (cmd == "broadcast")
            message = QString("This module can broadcast one important message to all group members.\\n"
                              "Note: Usable only by group's admin\\n"
                              "e.g. !broadcast message");
        else if (cmd == "tree")
            message = QString("This module shows a graph of group members.\\n"
                              "e.g. !tree");
        else if (cmd == "subscribe")
            message = QString("This module lets members subscribe to be notified when there's changes like "
                              "a new 'sup entry or a new poll.\\n"
                              "e.g. !subscribe subscribe\\n"
                              "e.g. !subscribe unsubscribe\\n"
                              "e.g. !subscribe");
        else if (cmd == "group")
            message = QString("This module lets members use group-only commands in pm.\\n"
                              "e.g. !group set 11558884\\n"
                              "e.g. !group unset\\n"
                              "e.g. !group (Lists those of your groups that bot manages)");
        else
            message = QString("Telegram-Bot (https://github.com/shervinkh/telegram-bot)\\n"
                              "Current modules are: calc stat help sup banlist poll broadcast tree "
                              "subscribe group\\n"
                              "Enter \"!help the_module\" (e.g. !help calc) for more help.");

        messageProcessor->sendCommand("msg " + identity.toUtf8() + " \"" + message.replace('"', "\\\"").toUtf8() + '"');
    }
}
