#include <QCoreApplication>
#include <QStringList>
#include "messageprocessor.h"
#include "signalhandler.h"
#include <QDebug>
#include <QDateTime>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (QCoreApplication::arguments().length() < 2)
        qFatal("Specify telegram-cli executable path.");

    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

    SignalHandler signalHandler;
    MessageProcessor MP;

    return a.exec();
}
