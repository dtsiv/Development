DESTDIR = $$PWD/../lib

CONFIG -= debug
CONFIG += release
CONFIG += static

OBJECTS_DIR= $$PWD/../build/.obj
MOC_DIR = $$PWD/../build/.moc
RCC_DIR = $$PWD/../build/.rcc
UI_DIR = $$PWD/../build/.ui

QT += core gui sql opengl

TEMPLATE = lib

QMAKE_CXXFLAGS += /std:c++17

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../../include/nr2 \
    ../QPropPages \
    ../QIniSettings \
    ../QPropPages \
    ../QExceptionDialog \
    ../QSqlModel \
    ../QRegFileParser \
    ../QPoi \
    ../QVoi \
    ../QIndicatorWindow
#    ../../CPP_211/other

HEADERS += \
    qtargetsmap.h \	
    qformular.h \
    qtargetmarker.h \
    mapwidget.h
	
SOURCES += \
    qtargetsmap.cpp \
    qformular.cpp \
    qtargetmarker.cpp \
    mapwidget.cpp
#    ../../CPP_211/recipes/sort.cpp
