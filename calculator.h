#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QMap>

class QProcess;
class QTimer;
class MessageProcessor;

class Calculator : public QObject
{
    Q_OBJECT
private:
    static const qint64 timeLimit;

    QMap<QProcess *, QByteArray> id;
    QMap<QProcess *, qint64> endTime;

    QTimer *checkEndTimer;

    MessageProcessor *messageProcessor;

public:
    explicit Calculator(MessageProcessor *msgProc, QObject *parent = 0);
    void input(const QString &gid, const QString &uid, const QString &msg, bool inpm);

public slots:
    void checkEndTime();
    void processCalculator();
};

#endif // CALCULATOR_H
