#-------------------------------------------------
#
# Project created by QtCreator 2017-12-24T13:20:20
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TwitchTest
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        #main.cpp \
        twitchtest.cpp \
    pingtest.cpp

HEADERS += \
        twitchtest.h \
    pingtest.h

INCLUDEPATH += deps

# LibRTMP files:
SOURCES += deps/librtmp/amf.c \
    deps/librtmp/cencode.c \
    deps/librtmp/hashswf.c \
    deps/librtmp/log.c \
    deps/librtmp/md5.c \
    deps/librtmp/parseurl.c \
    deps/librtmp/rtmp.c
