#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QMap>

class QProcess;
class QTimer;
class NameDatabase;
class MessageProcessor;
class Permission;

class Calculator : public QObject
{
    Q_OBJECT
private:
    static const qint64 timeLimit;

    QMap<QProcess *, QString> id;
    QMap<QProcess *, qint64> endTime;

    QTimer *checkEndTimer;

    NameDatabase *nameDatabase;
    MessageProcessor *messageProcessor;
    Permission *permission;

public:
    explicit Calculator(NameDatabase *namedb, MessageProcessor *msgProc, Permission *perm, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &msg, bool inpm, bool isAdmin);

public slots:
    void checkEndTime();
    void processCalculator();
};

#endif // CALCULATOR_H
