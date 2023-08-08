QT += testlib positioning
QT += gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += qt warn_on depend_includepath testcase

CONFIG += c++17
win32:QMAKE_CXXFLAGS += /std:c++17

#required to build interface for testing
DEFINES += IVS3DPLUGININTERFACE_LIBRARY

TEMPLATE = app

SOURCES +=  \
    logfile_stub.cpp \
    reader_stub.cpp \
    tst_nth_frame.cpp

HEADERS += \
    logfile_stub.h \
    reader_stub.h

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
    $$IVS_SRC_PATH/iVS3D-nthFramePlugin

HEADERS += \
    $$IVS_SRC_PATH/iVS3D-nthFramePlugin/nthframe.h \

SOURCES += \
    $$IVS_SRC_PATH/iVS3D-nthFramePlugin/nthframe.cpp \

unix {
 !include( ../../../setrpath.pri) {
   message("Cannot find setrpath.pri!")
 }
}
