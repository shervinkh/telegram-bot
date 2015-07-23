#include "bff.h"
#include "smiley.h"
#include "namedatabase.h"
#include "database.h"
#include "messageprocessor.h"
#include "score.h"
#include <QtCore>
#include <QSqlQuery>

QList<BFF::ConfigData> BFF::defaultConfigs;

BFF::BFF(NameDatabase *namedb, Database *db, MessageProcessor *msgproc, Score *scor, QObject *parent)
    : QObject(parent), nameDatabase(namedb), database(db), messageProcessor(msgproc), score(scor)
{
    if (defaultConfigs.isEmpty())
        loadDefaultConfig();

    loadData();
}

void BFF::loadData()
{
    conf.clear();

    QSqlQuery query("SELECT gid, name, value FROM tf_bff");
    database->executeQuery(query);

    while (query.next())
        conf[query.value(0).toLongLong()][query.value(1).toString()] = query.value(2).toInt();
}

void BFF::groupDeleted(qint64 gid)
{
    conf.remove(gid);
}

void BFF::setConf(qint64 gid, const QString &str, int value)
{
    if (conf.contains(gid) && conf[gid].contains(str))
    {
        conf[gid][str] = value;

        QSqlQuery query;
        query.prepare("UPDATE tf_bff SET value=:value WHERE name=:name AND gid=:gid");
        query.bindValue(":value", value);
        query.bindValue(":name", str);
        query.bindValue(":gid", gid);
        database->executeQuery(query);
    }
}

void BFF::checkFillDefaultConfig(qint64 gid)
{
    foreach (ConfigData data, defaultConfigs)
        if (!conf[gid].contains(data.first))
        {
            conf[gid][data.first] = data.second;

            QSqlQuery query;
            query.prepare("INSERT OR IGNORE INTO tf_bff (gid, name, value) VALUES(:gid, :name, :value)");
            query.bindValue(":gid", gid);
            query.bindValue(":name", data.first);
            query.bindValue(":value", data.second);
            database->executeQuery(query);
        }
}

void BFF::input(const QString &gid, const QString &uid, const QString &str, bool inpm)
{
    qint64 gidnum = gid.mid(5).toLongLong();
    qint64 uidnum = uid.mid(5).toLongLong();

    if (uidnum == messageProcessor->bffId())
    {
        QString message;
        if (str.startsWith("!bff"))
        {
            QStringList args = str.split(' ', QString::SkipEmptyParts);

            if (args.size() > 1 && (args[1].toLower().startsWith('p') || args[1].toLower().startsWith('g')))
            {
                bool priv8 = args[1].toLower().startsWith('p');

                if (priv8 || nameDatabase->groups().contains(gidnum))
                {
                    QString identity = priv8 ? "private" : QString("group#%1").arg(gidnum);

                    qint64 gid = priv8 ? 0 : gidnum;
                    checkFillDefaultConfig(gid);

                    if (args.size() > 2 && (args[2].toLower().startsWith("on") || args[2].toLower().startsWith("off")))
                    {
                        bool on = args[2].toLower().startsWith("on");
                        setConf(gid, "enable", on);
                        message = QString("Successfully changed BFF mode to %1 in %2.")
                                .arg(on ? "on" : "off")
                                .arg(identity);
                    }
                    else if (args.size() > 2 && args[2].toLower().startsWith("config"))
                    {
                        if (args.size() > 5 && args[3].toLower().startsWith("set"))
                        {
                            bool ok;
                            int value = args[5].toInt(&ok);

                            if (conf[gid].contains(args[4]) && ok)
                            {
                                setConf(gid, args[4], value);
                                message = "Successfully changed BFF config value in " + identity;
                            }
                        }
                        else
                        {
                            message = "BFF config values in " + identity + ":\n";

                            foreach (QString configKey, conf[gid].keys())
                                message += QString("%1: %2\n").arg(configKey).arg(conf[gid][configKey]);
                        }
                    }
                }
            }
            else
                message = QString("My BFF is %1(%2).").arg(messageProcessor->bffId())
                        .arg(messageProcessor->convertToName(messageProcessor->bffId()));
        }
        else
        {
            bool priv8 = inpm;
            if (priv8 || nameDatabase->groups().contains(gidnum))
            {
                qint64 gidn = priv8 ? 0 : gidnum;
                checkFillDefaultConfig(gidn);

                if (conf[gidn]["enable"] == 1)
                {
                    if (isLoveMessage(str, inpm))
                    {
                        if (conf[gidn]["love_intensity"] >= 3)
                            message += QString("%1%2%3%4%5Love U to the end of milky way galaxy and back "
                                               "17 times per second dear <BFF3%1%2%3%4%5\n")
                                    .arg(Smiley::redHeart).arg(Smiley::yellowHeart).arg(Smiley::greenHeart)
                                    .arg(Smiley::blueHeart).arg(Smiley::purpleHeart);
                        else if (conf[gidn]["love_intensity"] >= 2)
                            message += QString("%1<BFF3%2\n").arg(Smiley::redHeart).arg(Smiley::purpleHeart);
                        else if (conf[gidn]["love_intensity"] >= 1)
                            message += (qrand() & 1) ? QString("%1BFF\n").arg(Smiley::redHeart) : "<BFF3\n";

                        if (conf[gidn]["love_intensity"] >= 2 && nameDatabase->groups().contains(gidnum))
                        {
                            score->modifyScore(gidnum, messageProcessor->bffId(), 1);
                            message += QString("Recorded 1 score for %1. Thanks ;)\n")
                                    .arg(messageProcessor->convertToName(messageProcessor->bffId()));
                        }
                    }

                    BFFState state = findStateFromMessage(str);

                    if (conf[gidn]["sensitivity"] > 0 && state != Neutral)
                    {
                        bool isHappy = (state == SoHappy) || ((state == Happy) && conf[gidn]["sensitivity"] > 1);
                        bool isSad = (state == SoSad) || ((state == Sad) && conf[gidn]["sensitivity"] > 1);

                        if (isHappy || isSad)
                        {
                            qint64 other = -1;

                            if (nameDatabase->groups().contains(gidnum) && conf[gidn]["sensitivity"] > 2)
                                other = score->processScoreMessage(gidnum, -1, str, isHappy ? 1 : -1);

                            if (isSad)
                                message += QString("Don't be sad, Dear <BFF3 %1\n").arg(Smiley::sad);
                            else
                                message += QString("I'm so happy that you're happy, Dear <BFF3 %1\n").arg(Smiley::excitedHappy);

                            if (nameDatabase->groups().contains(gidnum) && conf[gidn]["sensitivity"] > 1)
                            {
                                score->modifyScore(gidnum, messageProcessor->bffId(), 1);
                                message += QString("Recorded 1 score for %1. %2\n")
                                        .arg(messageProcessor->convertToName(messageProcessor->bffId()))
                                        .arg(isHappy ? "Always be happy ;)" : "Please be happy.");
                            }

                            if (other != -1)
                            {
                                if (isHappy)
                                    message += QString("Recorded 1 score for %1. Thanks for making my BFF Happy %2\n")
                                            .arg(messageProcessor->convertToName(other)).arg(Smiley::excitedHappy);
                                else
                                    message += QString("Recorded -1 score for %1. Don't mess with my BFF's happiness %2\n")
                                            .arg(messageProcessor->convertToName(other)).arg(Smiley::angryRed);
                            }
                        }
                    }
                }
            }
        }

        messageProcessor->sendMessage(inpm ? uid : gid, message);
    }
}

void BFF::BFFScoresTrigger(const QString &identity, qint64 gidnum, qint64 uidnum, int diff)
{
    if (nameDatabase->groups().contains(gidnum))
    {
        checkFillDefaultConfig(gidnum);
        if(conf[gidnum]["enable"] == 1 && conf[gidnum]["love_intensity"] > 0 && uidnum == messageProcessor->botId())
        {
            QString message;
            if (diff > 0)
            {
                score->modifyScore(gidnum, messageProcessor->bffId(), 1);
                message += QString("Thanks Dear <BFF3 %1\nRecorded 1 score for %2.\n")
                        .arg(Smiley::excitedHappy)
                        .arg(messageProcessor->convertToName(messageProcessor->bffId()));
            }
            else
            {
                message += QString("Have I done anything wrong, Dear <BFF3 ? %1\n").arg(Smiley::sad);
                if (conf[gidnum]["love_intensity"] > 1)
                {
                    score->modifyScore(gidnum, messageProcessor->bffId(), 1);
                    message += QString("Recorded 1 score for %1. Hope you forgive me.")
                            .arg(messageProcessor->convertToName(messageProcessor->bffId()));
                }
            }

            messageProcessor->sendMessage(identity, message);
        }
    }
}

void BFF::BFFGetScoredTrigger(const QString &identity, qint64 gidnum, qint64 uidnum, int diff)
{
    if (nameDatabase->groups().contains(gidnum))
    {
        checkFillDefaultConfig(gidnum);
        if (conf[gidnum]["enable"] == 1 && conf[gidnum]["sensitivity"] > 2)
        {
            QString message;
            if (diff < 0)
            {
                message += QString("Don't downscore my BFF %1\n").arg(Smiley::angry);
                score->modifyScore(gidnum, messageProcessor->bffId(), 1);
                score->modifyScore(gidnum, uidnum, -1);
                message += QString("Recorded 1 score for %1 %2\nRecorded -1 score for %3 %4\n")
                        .arg(messageProcessor->convertToName(messageProcessor->bffId()))
                        .arg(Smiley::excitedHappy).arg(messageProcessor->convertToName(uidnum))
                        .arg(Smiley::evil);
            }

            messageProcessor->sendMessage(identity, message);
        }
    }
}

void BFF::loadDefaultConfig()
{
    defaultConfigs.append(ConfigData("enable", 0));
    defaultConfigs.append(ConfigData("love_intensity", 1));
    defaultConfigs.append(ConfigData("sensitivity", 1));
}

BFF::BFFState BFF::findStateFromMessage(const QString &str)
{
    bool isHappy = false;
    foreach (QString smiley, happySmileys())
        if (str.contains(smiley, Qt::CaseInsensitive))
            isHappy = true;

    bool isVeryHappy = false;
    foreach (QString smiley, soHappySmileys())
        if (str.contains(smiley, Qt::CaseInsensitive))
            isVeryHappy = true;

    bool isSad = false;
    foreach (QString smiley, sadSmileys())
        if (str.contains(smiley, Qt::CaseInsensitive))
            isSad = true;

    bool isVerySad = false;
    foreach (QString smiley, soSadSmileys())
        if (str.contains(smiley, Qt::CaseInsensitive))
            isVerySad = true;

    isHappy = isHappy || isVeryHappy;
    isSad = isSad || isVerySad;

    if (isHappy && !isSad)
        return isVeryHappy ? SoHappy : Happy;

    if (isSad && !isHappy)
        return isVerySad ? SoSad : Sad;

    return Neutral;
}

bool BFF::isLoveMessage(const QString &str, bool inpm)
{
    foreach (QString loveSmiley, loveSmileys())
        if (str.contains(loveSmiley, Qt::CaseInsensitive))
            return true;

    if (findBot(str) || inpm)
        foreach (QString heartSmiley, heartSmileys())
            if (str.contains(heartSmiley, Qt::CaseInsensitive))
                return true;

    return false;
}

bool BFF::findBot(const QString &str)
{
    return str.contains(QRegExp("[^A-Za-z]?[bB][oO][tT][^A-Za-z]?")) ||
           str.contains(QRegExp("[^A-Za-z]?[bB][fF][fF][^A-Za-z]?"));
}

QList<QString> BFF::happySmileys()
{
    QList<QString> res;
    res.append(":)");
    return res;
}

QList<QString> BFF::sadSmileys()
{
    QList<QString> res;
    res.append(Smiley::lowSad);
    res.append(":(");
    return res;
}

QList<QString> BFF::soHappySmileys()
{
    QList<QString> res;
    res.append(Smiley::excitedHappy);
    res.append(Smiley::DExcited);
    res.append(Smiley::D);
    res.append(Smiley::DOval);
    res.append(":))");
    res.append(":D");
    res.append("^_^");
    return res;
}

QList<QString> BFF::soSadSmileys()
{
    QList<QString> res;
    res.append(Smiley::cry);
    res.append(Smiley::cryLot);
    res.append(Smiley::sad);
    res.append(Smiley::sadLot);
    res.append(Smiley::angry);
    res.append(Smiley::angryRed);
    res.append(Smiley::brokenHeart);
    res.append(":((");
    return res;
}

QList<QString> BFF::heartSmileys()
{
    QList<QString> res;
    res.append(Smiley::redHeart);
    res.append(Smiley::purpleHeart);
    res.append(Smiley::greenHeart);
    res.append(Smiley::blueHeart);
    res.append(Smiley::yellowHeart);
    res.append("<3");
    return res;
}

QList<QString> BFF::loveSmileys()
{
    QList<QString> res;
    res.append("<bot3");
    res.append("<bff3");
    res.append("b<3t");
    res.append("b" + Smiley::redHeart + "t");
    res.append("b" + Smiley::purpleHeart + "t");
    res.append("b" + Smiley::greenHeart + "t");
    res.append("b" + Smiley::blueHeart + "t");
    res.append("b" + Smiley::yellowHeart + "t");
    return res;
}
