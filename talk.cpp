#include "talk.h"
#include "namedatabase.h"
#include "database.h"
#include "messageprocessor.h"
#include "permission.h"
#include "nickname.h"
#include <QtCore>
#include <QSqlQuery>

const int Talk::MaxSimilarContainLength = 100;
const qreal Talk::RecognitionThreshold = 0.7 ;
const int Talk::MaxTalksPerGroup = 50;

Talk::Talk(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Permission *perm, Nickname *nick, QObject *parent)
    : QObject(parent), nameDatabase(namedb), database(db), messageProcessor(msgproc), permission(perm), nickname(nick)
{
    loadData();
}

void Talk::loadData()
{
    data.clear();

    QSqlQuery query;
    query.prepare("SELECT id, gid, text, react FROM tf_talk");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 id = query.value(0).toLongLong();
        qint64 gid = query.value(1).toLongLong();
        QString text = query.value(2).toString();
        QString react = query.value(3).toString();

        data[gid][id] = TalkRecord(text, react);
    }
}

void Talk::groupDeleted(qint64 gid)
{
    data.remove(gid);
}

void Talk::input(const QString &gid, const QString &uid, const QString &str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum) && str.startsWith("!talk"))
    {
        QStringList args = str.split(' ', QString::SkipEmptyParts);

        QString message;
        int perm = 0;

        if (args.size() > 2 && args[1].toLower().startsWith("create"))
        {
            perm = permission->getPermission(gidnum, "talk", "create", isAdmin, inpm);

            if (perm == 1)
            {
                QString text = QStringList(args.mid(2)).join(" ");

                qint64 id = -1;
                if (data[gidnum].size() < MaxTalksPerGroup)
                    id = createTalk(gidnum, text);

                if (id != -1)
                    message = "Created new talk with id: " + QString::number(id);
                else
                    message = "Too much talks for this group!";
            }
        }
        else if (args.size() > 2 && (args[1].toLower().startsWith("del") || args[1].toLower().startsWith("rem")))
        {
            perm = permission->getPermission(gidnum, "talk", "delete", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                {
                    delTalk(gidnum, id);
                    message = QString("Deleted talk#%1.").arg(id);
                }
            }
        }
        else if (args.size() > 3 && args[1].toLower().startsWith("text"))
        {
            perm = permission->getPermission(gidnum, "talk", "text_edit", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                {
                    QString text = QStringList(args.mid(3)).join(" ");
                    editText(gidnum, id, text);
                    message = QString("Edited text for talk#%1.").arg(id);
                }
            }
        }
        else if (args.size() > 3 && args[1].toLower().startsWith("react"))
        {
            perm = permission->getPermission(gidnum, "talk", "react_edit", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                {
                    QString react = QStringList(args.mid(3)).join(" ");
                    editReact(gidnum, id, react);
                    message = QString("Edited react for talk#%1.").arg(id);
                }
            }
        }
        else if (args.size() > 2 && args[1].toLower().startsWith("show"))
        {
            inpm = inpm || (args.size() > 3 && args[3].startsWith("pm"));

            perm = permission->getPermission(gidnum, "talk", "show", isAdmin, inpm);

            if (perm == 1)
            {
                bool ok;
                qint64 id = args[2].toLongLong(&ok);

                if (ok && data[gidnum].contains(id))
                    message = QString("Talk#%1\nTrigger Text: %2\nReaction: %3").arg(id)
                            .arg(data[gidnum][id].text()).arg(data[gidnum][id].react());
            }
        }
        else
        {
            inpm = inpm || (args.size() > 1 && args[1].startsWith("pm"));

            perm = permission->getPermission(gidnum, "talk", "view_list", isAdmin, inpm);

            if (perm == 1)
            {
                if (data[gidnum].isEmpty())
                    message = "Nothing!";
                else
                {
                    int i = 0;
                    foreach (qint64 id, data[gidnum].keys())
                    {
                        message += QString("Talk#%1- %2").arg(id).arg(data[gidnum][id].text());

                        if (++i != data[gidnum].size())
                            message += '\n';
                    }
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);
    }

    if (nameDatabase->groups().keys().contains(gidnum) && uidnum != messageProcessor->botId())
    {
        int perm = permission->getPermission(gidnum, "talk", "talk", isAdmin, inpm);
        if (perm == 1)
        {
            QString message = findReact(gidnum, uidnum, str);
            messageProcessor->sendMessage(inpm ? uid : gid, message);
        }
    }
}

qint64 Talk::createTalk(qint64 gid, const QString &txt)
{
    QSqlQuery query;
    query.prepare("INSERT INTO tf_talk (gid, text) "
                  "VALUES(:gid, :text)");
    query.bindValue(":gid", gid);
    query.bindValue(":text", txt);
    database->executeQuery(query);

    if (query.lastInsertId().isValid())
    {
        qint64 id = query.lastInsertId().toLongLong();
        data[gid][id] = TalkRecord(txt, "");
        return id;
    }

    return -1;
}

void Talk::editText(qint64 gid, qint64 id, const QString &txt)
{
    QSqlQuery query;
    query.prepare("UPDATE tf_talk SET text=:text WHERE id=:id");
    query.bindValue(":text", txt);
    query.bindValue(":id", id);
    database->executeQuery(query);

    data[gid][id].text() = txt;
}

void Talk::editReact(qint64 gid, qint64 id, const QString &react)
{
    QSqlQuery query;
    query.prepare("UPDATE tf_talk SET react=:react WHERE id=:id");
    query.bindValue(":react", react);
    query.bindValue(":id", id);
    database->executeQuery(query);

    data[gid][id].react() = react;
}

void Talk::delTalk(qint64 gid, qint64 id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tf_talk WHERE id=:id");
    query.bindValue(":id", id);
    database->executeQuery(query);

    data[gid].remove(id);
}

QString Talk::findReact(qint64 gid, qint64 sender, const QString &text)
{
    QString answer;

    QStringList textWords = text.split(' ', QString::SkipEmptyParts);
    QString newText = textWords.join(" ");

    foreach (TalkRecord record, data[gid])
    {
        QString newRecordText = filterString(record.text());
        int matchCount = similarContain(filterString(newText), newRecordText);

        //qDebug() << "Test: " << filterString(newText) << newRecordText
        //         << static_cast<qreal>(matchCount) / newRecordText.length();

        if (static_cast<qreal>(matchCount) / newRecordText.length() > RecognitionThreshold)
        {
            if (!answer.isEmpty())
                answer += "\n";
            answer += processMessage(gid, sender, record.react());
        }
    }

    return answer;
}

QString Talk::processMessage(qint64 gid, qint64 senderid, const QString &text)
{
    QString result = text;

    QString sender = randomName(gid, senderid);
    result.replace("%s", sender);

    int idx = 0;
    do
    {
        idx = result.indexOf("%u", idx);

        if (idx != -1)
        {
            int endIdx = result.indexOf('u', idx + 2);

            if (endIdx != -1)
            {
                bool ok;
                qint64 userid = result.mid(idx + 2, endIdx - (idx + 2)).toLongLong(&ok);

                if (ok)
                {
                    QString username = randomName(gid, userid);
                    result.replace(idx, endIdx - idx + 1, username);
                    idx += username.length();
                }
                else
                    idx += 2;
            }
            else
                break;
        }
    } while (idx != -1);

    return result;
}

QString Talk::randomName(qint64 gid, qint64 uid)
{
    QList<QString> names = nickname->nickNames(gid, uid);
    names.append(messageProcessor->convertToName(uid));
    return names[qrand() % names.length()];
}

QString Talk::filterString(const QString &str)
{
    QString newStr;
    foreach (QChar ch, str)
        if (!ch.isPunct())
            newStr.append(ch);
        else
            newStr.append(' ');

    QStringList textWords = newStr.split(' ', QString::SkipEmptyParts);
    QString newText = textWords.join(" ");

    return newText;
}

int Talk::similarContain(QString str2, QString str1)
{
    static int answer[MaxSimilarContainLength][MaxSimilarContainLength];

    str1 = str1.toLower().mid(0, MaxSimilarContainLength);
    str2 = str2.toLower().mid(0, MaxSimilarContainLength);

    for (int i = 0; i < str1.length(); ++i)
        for (int j = 0; j < str2.length(); ++j)
            if (j == str2.length() - 1 || !str2[j + 1].isLetterOrNumber())
                answer[i][j] = (str1[i] == str2[j]);
            else
                answer[i][j] = 0;

    int maxLen = str1.length() / RecognitionThreshold;
    int totalAnswer = 0;
    for (int k = 0; k < maxLen - 1; ++k)
        for (int i = 0; i < str1.length(); ++i)
            for (int j = 0; j < str2.length(); ++j)
            {
                if (i != str1.length() - 1 && j != str2.length() - 1)
                    answer[i][j] = answer[i + 1][j + 1] + (str1[i] == str2[j]);
                if (i != str1.length() - 1)
                    answer[i][j] = qMax(answer[i][j], answer[i + 1][j]);
                if (j != str2.length() - 1)
                    answer[i][j] = qMax(answer[i][j], answer[i][j + 1]);

                if (j == 0 || !str2[j - 1].isLetterOrNumber())
                    totalAnswer = qMax(totalAnswer, answer[i][j]);
            }

    return totalAnswer;
}
