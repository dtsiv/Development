DESTDIR = $$PWD/../lib

CONFIG -= debug
CONFIG += release
CONFIG += static

OBJECTS_DIR= $$PWD/../build/.obj
MOC_DIR = $$PWD/../build/.moc
RCC_DIR = $$PWD/../build/.rcc
UI_DIR = $$PWD/../build/.ui

QT += core gui sql widgets

TEMPLATE = lib

QMAKE_CXXFLAGS += /std:c++17

HEADERS += \
    qexceptiondialog.h	
	
SOURCES += \
    qexceptiondialog.cpp
