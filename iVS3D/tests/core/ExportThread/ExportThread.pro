QT += core gui testlib concurrent

CONFIG += qt console warn_on depend_includepath testcase
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

SOURCES +=  tst_exportthread.cpp


DEFINES += IVS3DPLUGININTERFACE_LIBRARY


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

SOURCES += itransform_stub.cpp
HEADERS +=  itransform_stub.h

unix {
 !include( ../../../setrpath.pri) {
   message("Cannot find setrpath.pri!")
 }
}
