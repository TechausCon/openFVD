TEMPLATE = lib
CONFIG += staticlib
QT += core widgets
QT -= gui

TARGET = corelogic

INCLUDEPATH += $$PWD/../.. $$PWD/../../core

SOURCES += \
    ../../core/mnode.cpp \
    ../../core/exportfuncs.cpp

HEADERS += \
    ../../core/mnode.h \
    ../../core/exportfuncs.h \
    ../../lenassert.h
