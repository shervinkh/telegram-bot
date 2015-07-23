#include "score.h"
#include "namedatabase.h"
#include "database.h"
#include "messageprocessor.h"
#include "permission.h"
#include "smiley.h"
#include "nickname.h"
#include "bff.h"
#include <QtCore>
#include <QSqlQuery>

const int Score::MaxSimilarContainLength = 50;
const int Score::MessagesToKeep = 30;
const int Score::DaysToKeep = 7;
const qreal Score::RecognitionThreshold = 0.70;
const qreal Score::EPS = 1e-8;

Score::Score(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Permission *perm, Nickname *nck, QObject *parent)
    : QObject(parent), nameDatabase(namedb), database(db), messageProcessor(msgproc), permission(perm), nick(nck),
      bff(0)
{
    loadData();
}

void Score::loadData()
{
    scoreData.clear();

    QSqlQuery query;

    query.prepare("SELECT gid, uid, score FROM tf_scores");
    database->executeQuery(query);

    while (query.next())
    {
        qint64 gid = query.value(0).toLongLong();
        qint64 uid = query.value(1).toLongLong();
        qint64 score = query.value(2).toLongLong();

        scoreData[gid][uid] = score;
    }

    if (!scoreData[0].contains(0))
        scoreData[0][0] = QDate::currentDate().toJulianDay();
}

void Score::groupDeleted(qint64 gid)
{
    scoreData.remove(gid);
}

void Score::saveData()
{
    QMapIterator<qint64, UserScores> groupsIter(scoreData);
    while (groupsIter.hasNext())
    {
        groupsIter.next();
        qint64 gid = groupsIter.key();

        QMapIterator<qint64, qint64> usersIter(groupsIter.value());
        while (usersIter.hasNext())
        {
            usersIter.next();
            qint64 uid = usersIter.key();
            qint64 score = usersIter.value();

            QSqlQuery query;
            query.prepare("INSERT OR IGNORE INTO tf_scores (gid, uid) "
                          "VALUES(:gid, :uid)");
            query.bindValue(":gid", gid);
            query.bindValue(":uid", uid);
            database->executeQuery(query);

            query.prepare("UPDATE tf_scores SET score=:score WHERE gid=:gid AND uid=:uid");
            query.bindValue(":score", score);
            query.bindValue(":gid", gid);
            query.bindValue(":uid", uid);
            database->executeQuery(query);
        }
    }
}

QString Score::filterString(const QString &str)
{
    QString newStr;
    foreach (QChar ch, str)
        if (ch.isLetterOrNumber())
            newStr.append(ch);

    return newStr;
}

int Score::similarContain(QString str2, QString str1)
{
    static int answer[MaxSimilarContainLength][MaxSimilarContainLength];

    str1 = str1.toLower().mid(0, MaxSimilarContainLength);
    str2 = str2.toLower().mid(0, MaxSimilarContainLength);

    for (int i = 0; i < str1.length(); ++i)
        for (int j = 0; j < str2.length(); ++j)
            answer[i][j] = (str1[i] == str2[j]);

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
                totalAnswer = qMax(totalAnswer, answer[i][j]);
            }

    return totalAnswer;
}

int Score::isLikeOrDislike(QString &str)
{
    QStringList args = str.replace('\n', ' ').replace('\t', ' ').split(' ', QString::SkipEmptyParts);

    if (str.contains(Smiley::like))
    {
        str.remove(Smiley::like);
        return 1;
    }

    if (str.contains(Smiley::dislike))
    {
        str.remove(Smiley::dislike);
        return -1;
    }

    if (str.contains("++"))
    {
        str.remove("++");
        return 1;
    }

    if (str.contains("--"))
    {
        str.remove("--");
        return -1;
    }

    QString specialDashDash = QString() + QChar(8212);
    if (str.contains(specialDashDash))
    {
        str.remove(specialDashDash);
        return -1;
    }

    foreach (QString str, args)
    {
        QString lowered = str.toLower();

        bool like = (lowered == "like") || (lowered == "+1");
        bool dislike = (lowered == "dislike") || (lowered == "-1");

        if (like || dislike)
        {
            args.removeOne(str);
            str = args.join(" ");
            return like ? 1 : -1;
        }
    }

    return 0;
}

void Score::input(const QString &gid, const QString &uid, QString str, bool inpm, bool isAdmin)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (nameDatabase->groups().keys().contains(gidnum))
    {
        QString message;
        int perm = 0;

        if (str.startsWith("!score"))
        {
            QStringList args = str.split(' ', QString::SkipEmptyParts);

            if (args.size() > 2 && args[1].toLower().startsWith("view"))
            {
                inpm = inpm || (args.size() > 3 && args[3].toLower().startsWith("pm"));

                perm = permission->getPermission(gidnum, "score", "view_score", isAdmin, inpm);

                if (perm == 1)
                {
                    int start, end;
                    bool ok1, ok2;

                    if (args[2].toLower().startsWith("all"))
                    {
                        start = 1;
                        end = scoreData[gidnum].size();
                        ok1 = ok2 = true;
                    }
                    else
                    {
                        int idx = args[2].indexOf('-');

                        if (idx == -1)
                        {
                            start = end = args[2].toInt(&ok1);
                            ok2 = true;
                        }
                        else
                        {
                            start = args[2].mid(0, idx).toInt(&ok1);
                            end = args[2].mid(idx + 1).toInt(&ok2);
                        }
                    }

                    if (ok1 && ok2 && start > 0 && end > 0 && end >= start)
                    {
                        tempList.clear();

                        QMapIterator<qint64, qint64> scoresIter(scoreData[gidnum]);
                        while (scoresIter.hasNext())
                        {
                            scoresIter.next();
                            tempList.append(DataPair(scoresIter.value(), scoresIter.key()));
                        }

                        start = qMin(start, tempList.length());
                        end = qMin(end, tempList.length());

                        qSort(tempList.begin(), tempList.end(), qGreater<DataPair>());

                        QString startDate = QDate::fromJulianDay(scoreData[0][0]).toString("yyyy/MM/dd");
                        message = QString("Scoreboard (Since %1):").arg(startDate);
                        for (int i = start; i <= end; ++i)
                            message += QString("\n%1- %2 (%3)").arg(i)
                                       .arg(messageProcessor->convertToName(tempList[i - 1].second))
                                       .arg(tempList[i - 1].first);
                    }
                }
            }
            else if (args.size() > 3 && args[1].toLower().startsWith("change"))
            {
                perm = permission->getPermission(gidnum, "score", "direct_change_score", isAdmin, inpm);

                if (perm == 1)
                {
                    bool ok1, ok2;

                    qint64 uid = args[2].toLongLong(&ok1);
                    int diff = args[3].toInt(&ok2);

                    if (ok1 && ok2 && nameDatabase->userList(gidnum).contains(uid))
                    {
                        scoreData[gidnum][uid] += diff;
                        message = QString("Recorded %1 score for %2.")
                                .arg(diff)
                                .arg(messageProcessor->convertToName(uid));
                    }
                }
            }
        }

        int likeOrDislike = isLikeOrDislike(str);
        if (likeOrDislike && uidnum != messageProcessor->botId())
        {
            perm = permission->getPermission(gidnum, "score", "change_score", isAdmin, inpm);

            if (perm == 1)
            {
                qint64 tuid = processScoreMessage(gidnum, uidnum, str, likeOrDislike);
                if (tuid != -1)
                {
                    QString username = messageProcessor->convertToName(tuid);
                    message = "Recorded " + QString::number(likeOrDislike) + " score for " + username + ".";
                    if (uidnum == messageProcessor->bffId())
                        bff->BFFScoresTrigger(inpm ? uid : gid, gidnum, tuid, likeOrDislike);
                    if (tuid == messageProcessor->bffId())
                        bff->BFFGetScoredTrigger(inpm ? uid : gid, gidnum, uidnum, likeOrDislike);                    }
            }
        }

        MessageData newMsg(uidnum);
        msgs[gidnum].push_back(newMsg);

        if (msgs[gidnum].size() > MessagesToKeep)
            msgs[gidnum].pop_front();

        if (perm == 2)
            permission->sendRequest(gid, uid, str, inpm);

        messageProcessor->sendMessage(inpm ? uid : gid, message);
    }
}

void Score::dailyCron()
{
    if (QDate::currentDate().toJulianDay() - scoreData[0][0] >= DaysToKeep)
    {
        QSqlQuery query;
        query.prepare("DELETE FROM tf_scores");
        database->executeQuery(query);
        scoreData.clear();
        scoreData[0][0] = QDate::currentDate().toJulianDay();
    }
}

qint64 Score::processScoreMessage(qint64 gidnum, qint64 uidnum, const QString &str, int diff)
{
    int numMsgs = msgs[gidnum].size();
    int maxMatch = 0;
    qreal maxRatio = 0;
    int matchIdx = -1;
    QString filteredStr = filterString(str);

    for (int i = numMsgs - 1; i >= 0; --i)
    {
        qint64 tuid = msgs[gidnum][i].uid();

        if (tuid != uidnum && !msgs[gidnum][i].likedUsers().contains(uidnum))
        {
            QList<QString> usernames = nick->nickNames(gidnum, tuid);
            usernames.prepend(messageProcessor->convertToName(tuid));

            foreach (QString username, usernames)
            {
                QString firstName;
                int firstNameIdx = username.indexOf(' ');
                if (firstNameIdx != -1)
                    firstName = filterString(username.mid(0, firstNameIdx));

                username = filterString(username);

                int recognition = similarContain(filteredStr, username);
                qreal ratio = static_cast<qreal>(recognition) / username.size();

                if (!firstName.isEmpty())
                {
                    int newRecognition = similarContain(filteredStr, firstName);
                    qreal newRatio = static_cast<qreal>(newRecognition) / firstName.size();

                    if (newRecognition > recognition || (newRecognition == recognition && newRatio > ratio)
                        || (newRatio + EPS > RecognitionThreshold && ratio + EPS < RecognitionThreshold))
                    {
                        recognition = newRecognition;
                        ratio = newRatio;
                    }
                }

                if (recognition > maxMatch || (recognition == maxMatch && ratio > maxRatio)
                    || (ratio + EPS > RecognitionThreshold && maxRatio + EPS < RecognitionThreshold))
                {
                    maxMatch = recognition;
                    maxRatio = ratio;
                    matchIdx = i;
                }
            }
        }
    }

    if (matchIdx != -1 && maxRatio + EPS > RecognitionThreshold)
    {
        qint64 tuid = msgs[gidnum][matchIdx].uid();

        if (uidnum != -1)
            msgs[gidnum][matchIdx].likedUsers().append(uidnum);

        scoreData[gidnum][tuid] += diff;

        return tuid;
    }

    return -1;
}

void Score::modifyScore(qint64 gidnum, qint64 uidnum, int diff)
{
    if (nameDatabase->groups().keys().contains(gidnum) &&
        nameDatabase->userList(gidnum).keys().contains(uidnum))
        scoreData[gidnum][uidnum] += diff;
}
