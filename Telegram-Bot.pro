#-------------------------------------------------
#
# Project created by QtCreator 2014-10-28T22:25:57
#
#-------------------------------------------------

QT       += core sql

QT       -= gui

TARGET = Telegram-Bot
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    messageprocessor.cpp \
    calculator.cpp \
    database.cpp \
    statistics.cpp \
    namedatabase.cpp \
    signalhandler.cpp \
    help.cpp \
    sup.cpp \
    banlist.cpp \
    poll.cpp \
    broadcast.cpp \
    tree.cpp

HEADERS += \
    messageprocessor.h \
    calculator.h \
    database.h \
    statistics.h \
    namedatabase.h \
    signalhandler.h \
    help.h \
    sup.h \
    banlist.h \
    poll.h \
    broadcast.h \
    tree.h
