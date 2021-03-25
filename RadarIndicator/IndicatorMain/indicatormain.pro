TARGET = RadarIndicator

DESTDIR = $$PWD/../

TEMPLATE = app

QMAKE_CXXFLAGS += /std:c++17

RadarIndicator.depends = $$PWD/../QIndicatorWindow

OBJECTS_DIR= $$PWD/../build/.obj
MOC_DIR = $$PWD/../build/.moc
RCC_DIR = $$PWD/../build/.rcc
UI_DIR = $$PWD/../build/.ui

QT += core gui widgets sql

CONFIG -= debug
CONFIG += release
CONFIG += static

CONFIG -= import_plugins

INCLUDEPATH +=  $PWD/include

INCLUDEPATH += \
    ../include \
    ../QIndicatorWindow \
    ../QIniSettings \
    ../QPropPages \
    ../QSqlModel \
    ../QRegFileParser \
    ../QTargetsMap \
    ../QPoi \
    ../QVoi \
    $$PWD/../../include/nr2 \
    ../QExceptionDialog

SOURCES += main.cpp

RESOURCES += \
    indicatormain.qrc

LIBS += -L..\lib \
    -lqindicatorwindow \
    -lqinisettings \
    -lqproppages \
    -lqsqlmodel \
    -lqregfileparser \
    -lqtargetsmap \
    -lqexceptiondialog \
    -lqpoi \
    -lqvoi


win32 {
   RC_FILE = indicatormain.rc
#   QMAKE_LFLAGS_WINDOWS += /VERBOSE
}

