#include <QCoreApplication>
#include <QStringList>
#include "messageprocessor.h"
#include "signalhandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (QCoreApplication::arguments().length() < 3)
        qFatal("Specify telegram and python3 executable path.");

    SignalHandler signalHandler;
    MessageProcessor MP;

    return a.exec();
}
