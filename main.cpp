#include <QCoreApplication>
#include <QStringList>
#include "messageprocessor.h"
#include "signalhandler.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (QCoreApplication::arguments().length() < 2)
        qFatal("Specify telegram-cli executable path.");

    SignalHandler signalHandler;
    MessageProcessor MP;

    return a.exec();
}
