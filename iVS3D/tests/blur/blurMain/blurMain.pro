QT += core gui testlib concurrent

CONFIG += qt console warn_on depend_includepath testcase
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

#required to build interface for testing
DEFINES += IVS3DPLUGININTERFACE_LIBRARY

# testfiles
HEADERS += \
    reader_stub.h \
    logfile_stub.h

SOURCES += \
    tst_blurMain.cpp \
    reader_stub.cpp \
    logfile_stub.cpp

# -----------   opencv   -----------------------------------------------
#
# the opencv headers and bins are included using the local 3rdparty.pri file.
# these are required if the tested files use opnecv classes such as cv::Mat or
# use opencv functionality.
include(../../../3rdparty.pri)

# -----------   resourceloader.pri   -------------------------------------
#
# resourceloader.pri holdes an absolute path to the testresources folder.
# to use it in the .cpp files, the path is added to the preprocessesor macros
# as TEST_RESOURCES. This macro holds a String to the testresources folder.
# It also adds the resourceloader.h file to the HEADERS list in order to use the
# requireResource function.
include(../../resourceloader.pri)
DEFINES += TEST_RESOURCES=\"\\\"$${TEST_RESOURCES_PATH}\\\"\"

# -----------   files to test   ------------------------------------------
#
# these files and there dependencies are tested within this unit test
INCLUDEPATH += \
    $$IVS_SRC_PATH/iVS3D-core/plugin \
    $$IVS_SRC_PATH/iVS3D-core/model \
    $$IVS_SRC_PATH/iVS3D-pluginInterface \
    $$IVS_SRC_PATH/iVS3D-blurPlugin

HEADERS += \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blur.h \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/BlurAlgorithm.h \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blursobel.h \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blurlaplacian.h \
    $$IVS_SRC_PATH/iVS3D-pluginInterface/ialgorithm.h \
    $$IVS_SRC_PATH/iVS3D-core/model/reader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/progressable.h \

SOURCES += \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blur.cpp \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/BlurAlgorithm.cpp \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blursobel.cpp \
    $$IVS_SRC_PATH/iVS3D-blurPlugin/blurlaplacian.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/progressable.cpp
