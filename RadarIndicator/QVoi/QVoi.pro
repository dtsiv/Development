DESTDIR = $$PWD/../lib

CONFIG -= debug
CONFIG += release
CONFIG += static

OBJECTS_DIR= $$PWD/../build/.obj
MOC_DIR = $$PWD/../build/.moc
RCC_DIR = $$PWD/../build/.rcc
UI_DIR = $$PWD/../build/.ui

QT += core gui

TEMPLATE = lib

QMAKE_CXXFLAGS += /std:c++17

QMAKE_CXXFLAGS += /wd4018 /wd4244

INCLUDEPATH += \
    ../include \
    ../QPropPages \
    ../QExceptionDialog \
    ../QIniSettings \
    ../QExceptionDialog \
    $$PWD/../../include/nr2 \
    $$PWD/../../include

HEADERS += \
    qvoi.h \
    qcoretracefilter.h \
    qtracefilter.h \
    kalextended.h \
    qsimplefilter.h

VPATH += ../NRSources
SOURCES += \
    gaussj.cpp \
    betacf.cpp \
    betai.cpp \
    gammln.cpp \
    ttest.cpp \
    avevar.cpp \
    crank.cpp \
    erfcc.cpp \
    sort2.cpp \
    spear.cpp \
    qvoi.cpp \
    qcoretracefilter.cpp \
    qtracefilter.cpp \
    kalextended.cpp \
    qsimplefilter.cpp
