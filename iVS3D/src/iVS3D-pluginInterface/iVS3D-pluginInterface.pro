#QT -= gui

TARGET = ITransform

TEMPLATE = lib
DEFINES += IVS3DPLUGININTERFACE_LIBRARY

CONFIG += plugin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../iVS3D-core

SOURCES +=

HEADERS += \
    iVS3D-pluginInterface_global.h \
    itransform.h \
    cvmat_qmetadata.h

unix {
 !include( ../../setrpath.pri) {
   message("Cannot find setrpath.pri!")
 }
}

CONFIG(debug, debug|release){
    VARIANT = debug
} else {
    VARIANT = release
}

win32:DESTDIR = $$OUT_PWD/../iVS3D-core/$$VARIANT
else:unix:DESTDIR = $$OUT_PWD/../iVS3D-core

include(../../3rdparty.pri)
