load(common_pre)

QT       += core dbus gui widgets network xml

TARGET = simulatoremulated
TEMPLATE = lib

CONFIG += plugin shared
CONFIG += blackmisc blackcore blackgui

DEPENDPATH  += . $$SourceRoot/src
INCLUDEPATH += . $$SourceRoot/src

SOURCES += *.cpp
HEADERS += *.h
FORMS   += *.ui
DISTFILES += simulatoremulated.json
DESTDIR = $$DestRoot/bin/plugins/simulator

LIBS *= -lsimulatorplugincommon
addStaticLibraryDependency(simulatorplugincommon)

win32 {
    dlltarget.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += dlltarget
} else {
    target.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += target
}

load(common_post)
