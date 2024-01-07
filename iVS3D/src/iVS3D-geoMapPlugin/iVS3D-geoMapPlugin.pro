QT += core gui concurrent qml network quick positioning location

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += \
    ../iVS3D-pluginInterface \
    ../iVS3D-core \
    ../iVS3D-core/model \
    ../iVS3D-core/model/reader \
    ../iVS3D-core/model/metaData \
    ../iVS3D-core/model/log \
    ../iVS3D-core/plugin


include(../../3rdparty.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    geomap.cpp \
    maphandler.cpp


HEADERS += \
    geomap.h \
    maphandler.h



TEMPLATE = lib
CONFIG += plugin

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../iVS3D-core/release -liVS3D-pluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../iVS3D-core/debug -liVS3D-pluginInterface
else:unix: LIBS += -L$$OUT_PWD/../iVS3D-core -liVS3D-pluginInterface

# create .dll in plugins folder
#TARGET = $$qtLibraryTarget(GeoMapPlugin)
TARGET = GeoMap
DESTDIR = ../plugins


FORMS +=

DISTFILES += \
    map.qml \
    mapmarker.qml

TRANSLATIONS += \
    $$PWD/translations/geomap_en.ts \
    $$PWD/translations/geomap_de.ts

unix:system(lrelease $$PWD/translations/*.ts)
win32:system(lrelease-pro iVS3D-geoMapPlugin.pro)

RESOURCES += \
    geomap.qrc

unix {
# set rpath
RPATH='\$$ORIGIN:\$$ORIGIN/../lib'
QMAKE_LFLAGS += '-Wl,--rpath=\'$$RPATH\''
}
