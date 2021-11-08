QT += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

INCLUDEPATH += \
    ../iVS3D-core/plugin \
    ../iVS3D-core/model

include(../../3rdparty.pri)

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
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# create .dll in plugins folder
TARGET = $$qtLibraryTarget(blurPlugin)
DESTDIR = ../plugins



FORMS +=
