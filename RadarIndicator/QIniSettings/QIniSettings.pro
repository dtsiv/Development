DESTDIR = $$PWD/../lib

CONFIG -= debug
CONFIG += release
CONFIG += static

OBJECTS_DIR= $$PWD/../build/.obj
MOC_DIR = $$PWD/../build/.moc
RCC_DIR = $$PWD/../build/.rcc
UI_DIR = $$PWD/../build/.ui

QT += core

TEMPLATE = lib

QMAKE_CXXFLAGS += /std:c++17

HEADERS += \
    qinisettings.h	\
	qserial.h
	
SOURCES += \
    qinisettings.cpp \
	qserial.cpp
