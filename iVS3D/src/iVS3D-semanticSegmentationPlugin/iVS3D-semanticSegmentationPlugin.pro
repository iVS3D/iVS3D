QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += plugin

TARGET = SemanticSegmentation

DEFINES += IVS3DSEMANTICSEGMENTATIONPLUGIN_LIBRARY
TEMPLATE = lib

INCLUDEPATH += $$PWD/../iVS3D-core

SOURCES += \
    semanticsegmentation.cpp \
    settingswidget.cpp

HEADERS += \
    semanticsegmentation.h \
    semanticsegmentation_global.h \
    settingswidget.h \
    cvmat_qmetadata.h

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../iVS3D-pluginInterface/lib/ -lITransform
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../iVS3D-pluginInterface/lib/ -lITransform
else:unix: LIBS += -L$$PWD/../iVS3D-pluginInterface/lib/ -lITransform

INCLUDEPATH += $$PWD/../iVS3D-pluginInterface
DEPENDPATH  += $$PWD/../iVS3D-pluginInterface

DESTDIR = ../plugins

include(../../3rdparty.pri)

FORMS +=

CONFIG(debug, debug|release){
    VARIANT = debug
} else {
    VARIANT = release
}


win32: model_files.path = $$OUT_PWD/../iVS3D-core/$$VARIANT/models/SemanticSegmentation
else:unix: model_files.path = $$OUT_PWD/../iVS3D-core/models/SemanticSegmentation
model_files.files = copy_to_install_dir/models/SemanticSegmentation/*

INSTALLS += model_files

