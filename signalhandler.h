//This class converts low-level SIGINT and SIGTERM signals
//to high-level Qt signals and it has two handler for these
//two signals to exit program properly

#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <QObject>

class QSocketNotifier;

class SignalHandler : public QObject
{
    Q_OBJECT
private:
    static int sigintFd[2];
    static int sigtermFd[2];

    QSocketNotifier *snInt;
    QSocketNotifier *snTerm;

public:
    explicit SignalHandler(QObject *parent = 0);

    // Signal handlers
    static void intSignalHandler(int);
    static void termSignalHandler(int);
    
public slots:
    // Qt Signal Handlers
    void handleSigInt();
    void handleSigTerm();
};

#endif // SIGNALHANDLER_H
