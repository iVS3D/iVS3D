QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_projectmanager.cpp

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
    $$IVS_SRC_PATH/iVS3D-core/model \

HEADERS += \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.h \
    $$IVS_SRC_PATH/iVS3D-core/model/modelalgorithm.h \
    $$IVS_SRC_PATH/iVS3D-core/model/jsonEnum.h \
    $$IVS_SRC_PATH/iVS3D-core/model/concurrentreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.h \


SOURCES += \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/modelalgorithm.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/jsonEnum.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/concurrentreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.cpp \
