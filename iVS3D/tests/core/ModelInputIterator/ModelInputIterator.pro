QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

#required to build interface for testing
DEFINES += IVS3DPLUGININTERFACE_LIBRARY

SOURCES +=  tst_modelinputiterator.cpp

# -----------   opencv   -----------------------------------------------
#
# the opencv headers and bins are included using the local 3rdparty.pri file.
# these are required if the tested files use opnecv classes such as cv::Mat or
# use opencv functionality.
include(../../../3rdparty.pri)

# resourceloader.pri holdes an absolute path to the testresources folder.
# to use it in the .cpp files, the path is added to the preprocessesor macros
# as TEST_RESOURCES. This macro holds a String to the testresources folder.
# It also adds the resourceloader.h file to the HEADERS list in order to use the
# requireResource function.
include(../../resourceloader.pri)
DEFINES += TEST_RESOURCES=\"\\\"$${TEST_RESOURCES_PATH}\\\"\"

# files to test
INCLUDEPATH += \
    $$IVS_SRC_PATH/iVS3D-core \
    $$IVS_SRC_PATH/iVS3D-core/model \
    $$IVS_SRC_PATH/iVS3D-core/plugin \
    $$IVS_SRC_PATH/iVS3D-core/controller \
    $$IVS_SRC_PATH/iVS3D-pluginInterface

HEADERS += \
    $$IVS_SRC_PATH/iVS3D-core/controller/modelinputiteratorfactory.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/ModelInputIterator.h \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/imageiterator.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/keyframeiterator.h \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreaer.h \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/ConcurrentReader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/signalobject.h \
    $$IVS_SRC_PATH/iVS3D-core/model/metadatamanager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/metadata.h \
    $$IVS_SRC_PATH/iVS3D-core/model/metadatareader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreaderdji.h \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreaderexif.h \
    $$IVS_SRC_PATH/iVS3D-core/model/exif.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/algorithmmanager.h \
    $$IVS_SRC_PATH/iVS3D-pluginInterface/ialgorithm.h

SOURCES += \
    $$IVS_SRC_PATH/iVS3D-core/controller/modelinputiteratorfactory.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/imageiterator.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/keyframeiterator.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/ConcurrentReader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.cpp \
    $$IVS_SRC_PATH/iVS3D-core/plugin/algorithmmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/plugin/signalobject.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/metadatamanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreaderdji.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/gpsreaderexif.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/exif.cpp
