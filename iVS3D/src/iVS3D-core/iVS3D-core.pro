QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += console

RC_ICONS = resources/ivs3dIcon.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += \
    blurfilter \
    controller \
    model \
    plugin \
    view \
    $$_PRO_FILE_PWD_

SOURCES += \
    controller/nouicontroller.cpp \
    main.cpp \
# controller package
    controller/controller.cpp \
    controller/imageiterator.cpp \
    controller/keyframeiterator.cpp \
    controller/modelinputiteratorfactory.cpp \
    controller/videoplayercontroller.cpp \
    controller/algorithmcontroller.cpp \
    controller/exportcontroller.cpp \
# plugin package
    model/automaticexecsettings.cpp \
    model/automaticexecutor.cpp \
    model/concurrentreader.cpp \
    model/exif.cpp \
    model/gpsreader.cpp \
    model/gpsreaderdji.cpp \
    model/gpsreaderexif.cpp \
    model/history.cpp \
    model/metadatamanager.cpp \
    model/nouiexport.cpp \
    model/progressable.cpp \
    model/progressdisplay.cpp \
    model/samplethread.cpp \
    model/settingsthread.cpp \
    model/stringcontainer.cpp \
    plugin/itransformrequestdequeue.cpp \
    plugin/signalobject.cpp \
    plugin/transformmanager.cpp \
    plugin/algorithmmanager.cpp \
# view package
    view/automaticlistwidget.cpp \
    view/automaticwidget.cpp \
    view/cropexport.cpp \
    view/exportwidget.cpp \
    view/helpdialog.cpp \
    view/inputautomaticwidget.cpp \
    view/licencedialog.cpp \
    view/outputwidget.cpp \
    view/progressdialog.cpp \
    view/progresswidget.cpp \
    view/reallydeletedialog.cpp \
    view/reconstructiontoolsdialog.cpp \
    view/roiselect.cpp \
    view/samplingwidget.cpp \
    view/slideablelabel.cpp \
    view/terminalinteraction.cpp \
    view/timeline.cpp \
    view/timelinelabel.cpp \
    view/mainwindow.cpp \
    view/about.cpp \
    view/videoplayer.cpp \
    view/infowidget.cpp \
    view/darkstyle/DarkStyle.cpp \
    view/reconstructdialog.cpp\
    view/emptyfolderdialog.cpp \
# model package
    model/logfile.cpp \
    model/logmanager.cpp \
    model/delayedcopyreader.cpp \
    model/openexecutor.cpp \
    model/exportexecutor.cpp \
    model/imagereader.cpp \
    model/videoreader.cpp \
    model/exportthread.cpp \
    model/projectmanager.cpp \
    model/applicationsettings.cpp \
    model/modelalgorithm.cpp \
    model/modelinputpictures.cpp \
    model/DataManager.cpp \
    model/algorithmthread.cpp \
    model/algorithmexecutor.cpp

HEADERS += \
# controller package
    controller/ModelInputIterator.h \
    controller/imageiterator.h \
    controller/keyframeiterator.h \
    controller/modelinputiteratorfactory.h \
    controller/nouicontroller.h \
    controller/videoplayercontroller.h \
    controller/controller.h \
    controller/algorithmcontroller.h \
    controller/exportcontroller.h \
# plugin package
    cvmat_qmetadata.h \
    model/LogFileParent.h \
    model/automaticexecsettings.h \
    model/automaticexecutor.h \
    model/concurrentreader.h \
    model/exif.h \
    model/gpsreader.h \
    model/gpsreaderdji.h \
    model/gpsreaderexif.h \
    model/history.h \
    model/metadata.h \
    model/metadatamanager.h \
    model/metadatareader.h \
    model/nouiexport.h \
    model/progressdisplay.h \
    model/samplethread.h \
    model/settingsthread.h \
    model/stringcontainer.h \
    plugin/algorithmmanager.h \
    plugin/itransformrequestdequeue.h \
    plugin/signalobject.h \
    plugin/transformmanager.h \
# view package
    view/automaticlistwidget.h \
    view/automaticwidget.h \
    view/cropexport.h \
    view/exportwidget.h \
    view/helpdialog.h \
    view/inputautomaticwidget.h \
    view/licencedialog.h \
    view/mainwindow.h \
    view/about.h \
    view/outputwidget.h \
    view/progressdialog.h \
    view/progresswidget.h \
    view/reallydeletedialog.h \
    view/reconstructiontoolsdialog.h \
    view/roiselect.h \
    view/samplingwidget.h \
    view/terminalinteraction.h \
    view/videoplayer.h \
    view/infowidget.h \
    view/slideablelabel.h \
    view/timeline.h \
    view/timelinelabel.h \
    view/darkstyle/DarkStyle.h \
    view/reconstructdialog.h \
    view/emptyfolderdialog.h \
# model package
    model/logfile.h \
    model/logmanager.h \
    model/delayedcopyreader.h \
    model/openexecutor.h \
    model/ISerializable.h \
    model/exportthread.h \
    model/imagereader.h \
    model/progressable.h \
    model/reader.h \
    model/videoreader.h \
    model/exportexecutor.h \
    model/applicationsettings.h \
    model/modelalgorithm.h \
    model/modelinputpictures.h \
    model/DataManager.h \
    model/projectmanager.h \
    model/algorithmthread.h \
    model/algorithmexecutor.h

FORMS += \
    view/automaticwidget.ui \
    view/cropexport.ui \
    view/exportwidget.ui \
    view/helpdialog.ui \
    view/inputautomaticwidget.ui \
    view/licencedialog.ui \
    view/progressdialog.ui \
    view/progresswidget.ui \
    view/reallydeletedialog.ui \
    view/reconstructiontoolsdialog.ui \
    view/samplingwidget.ui \
    view/timeline.ui \
    view/mainwindow.ui \
    view/about.ui \
    view/videoplayer.ui \
    view/infowidget.ui \
    view/reconstructdialog.ui \
    view/emptyfolderdialog.ui

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/release -liVS3D-pluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/debug -liVS3D-pluginInterface
else:unix: LIBS += -L$$OUT_PWD -liVS3D-pluginInterface

INCLUDEPATH += $$PWD/../iVS3D-pluginInterface
DEPENDPATH  += $$PWD/../iVS3D-pluginInterface

# Include opencv using a local 3rdparty.pri
include(../../3rdparty.pri)

DISTFILES +=

RESOURCES += \
    darkstyle.qrc \
    resources.qrc

CONFIG(debug, debug|release){
    VARIANT = debug
} else {
    VARIANT = release
}

win32:colmap_files.path = $$OUT_PWD/$$VARIANT/colmap
else:unix:colmap_files.path = $$OUT_PWD/colmap
colmap_files.files = copy_to_install_dir/colmap/*
INSTALLS += colmap_files

with_cuda{
    DEFINES += WITH_CUDA

    with_dependencies{
        #if installing, copy cuda dlls to install dir
        cuda_dlls.path = $$OUT_PWD/$$VARIANT
        for(file, CUDA_BIN_FILES){
            cuda_dlls.files += $$CUDA_BIN_PATH/$${file}
            exists($$CUDA_BIN_PATH/$${file}):message(CUDA FILE FOUND: $$CUDA_BIN_PATH/$${file})
            else:warning(CUDA FILE MISSING: $$CUDA_BIN_PATH/$${file})
        }
        INSTALLS += cuda_dlls
    }
}

with_dependencies{
    #if installing, copy msvc2015 dlls to install dir
    msvc_dlls.path = $$OUT_PWD/$$VARIANT
    for(file, MSVC_BIN_FILES){
        msvc_dlls.files += $$MSVC_BIN_PATH/$${file}
        exists($$MSVC_BIN_PATH/$${file}):message(MSVC FILE FOUND: $$MSVC_BIN_PATH/$${file})
        else:warning(MSVC FILE MISSING: $$MSVC_BIN_PATH/$${file})
    }
    INSTALLS += msvc_dlls

    #if installing, copy opencv dlls to install dir
    opencv_dlls.path = $$OUT_PWD/$$VARIANT
    for(file, OPENCV_BIN_FILES){
        opencv_dlls.files += $$OPENCV_BIN_PATH/$${file}
        exists($$OPENCV_BIN_PATH/$${file}):message(OPENCV FILE FOUND: $$OPENCV_BIN_PATH/$${file})
        else:warning(OPENCV FILE MISSING: $$OPENCV_BIN_PATH/$${file})
    }
    INSTALLS += opencv_dlls
}

unix {
 !include( ../../setrpath.pri) {
   message("Cannot find setrpath.pri!")
 }
}
