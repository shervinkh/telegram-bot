#include "signalhandler.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QDebug>
#include <QSocketNotifier>

int SignalHandler::sigintFd[2];
int SignalHandler::sigtermFd[2];

static int setup_unix_signal_handlers()
{
    struct sigaction intt, term;

    intt.sa_handler = SignalHandler::intSignalHandler;
    sigemptyset(&intt.sa_mask);
    intt.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &intt, 0) > 0)
        return 1;

    term.sa_handler = SignalHandler::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, 0) > 0)
        return 2;

    return 0;
}

SignalHandler::SignalHandler(QObject *parent) :
    QObject(parent)
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
        qFatal("Couldn't create INT socketpair");

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");

    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));

    setup_unix_signal_handlers();
}

void SignalHandler::intSignalHandler(int)
{
    char tmp = 1;
    write(sigintFd[0], &tmp, sizeof tmp);
}

void SignalHandler::termSignalHandler(int)
{
    char tmp = 1;
    write(sigtermFd[0], &tmp, sizeof tmp);
}

void SignalHandler::handleSigInt()
{
    snTerm->setEnabled(false);
    char tmp;
    read(sigintFd[1], &tmp, sizeof tmp);

    QCoreApplication::exit(EXIT_SUCCESS);

    snTerm->setEnabled(true);
}

void SignalHandler::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    read(sigtermFd[1], &tmp, sizeof tmp);

    QCoreApplication::exit(EXIT_SUCCESS);

    snTerm->setEnabled(true);
}
