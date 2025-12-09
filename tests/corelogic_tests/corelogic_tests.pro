TEMPLATE = app
QT += testlib core widgets
QT -= gui
CONFIG += c++11

TARGET = corelogic_tests

SOURCES += \
    corelogic_tests.cpp

INCLUDEPATH += $$PWD/../.. $$PWD/../../core

win32:LIBS += -L$$OUT_PWD/../corelogic/release -lcorelogic
else:LIBS += -L$$OUT_PWD/../corelogic -lcorelogic

macx:DEFINES += QT_NO_SHORTCUT

depends = ../corelogic
