QT += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

INCLUDEPATH += \
    ../iVS3D-pluginInterface \
    ../iVS3D-core \
    ../iVS3D-core/model \
    ../iVS3D-core/model/reader \
    ../iVS3D-core/model/metaData \
    ../iVS3D-core/model/log \
    ../iVS3D-core/plugin

include(../../3rdparty.pri)

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../iVS3D-core/release -liVS3D-pluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../iVS3D-core/debug -liVS3D-pluginInterface
else:unix: LIBS += -L$$OUT_PWD/../iVS3D-core -liVS3D-pluginInterface


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    BlurAlgorithm.cpp \
    blur.cpp \
    blurlaplacian.cpp \
    blursobel.cpp

HEADERS += \
    BlurAlgorithm.h \
    blur.h \
    blurlaplacian.h \
    blursobel.h


TEMPLATE = lib
CONFIG += plugin

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

TARGET = Blur
# create .dll in plugins folder
#TARGET = $$qtLibraryTarget(blurPlugin)
DESTDIR = ../plugins


FORMS +=

unix {
# set rpath
RPATH='\$$ORIGIN:\$$ORIGIN/../lib'
QMAKE_LFLAGS += '-Wl,--rpath=\'$$RPATH\''
}

TRANSLATIONS += \
    $$PWD/translations/blur_en.ts \
    $$PWD/translations/blur_de.ts

RESOURCES += \
    resources.qrc
