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
    target.path = /usr/lib
    INSTALLS += target
}

DESTDIR = $$PWD/lib

include(../../3rdparty.pri)

CONFIG(debug, debug|release){
    VARIANT = debug
} else {
    VARIANT = release
}

model_files.path = $$OUT_PWD/../iVS3D-core/$$VARIANT
model_files.files = $$PWD/lib/ITransform.dll

INSTALLS += model_files
