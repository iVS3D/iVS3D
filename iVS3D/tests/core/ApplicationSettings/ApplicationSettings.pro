QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_applicationsettings.cpp

# opencv
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
    $$IVS_SRC_PATH/iVS3D-core/model

HEADERS += \
    $$IVS_SRC_PATH/iVS3D-core/model/applicationsettings.h \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.h

SOURCES += \
    $$IVS_SRC_PATH/iVS3D-core/model/applicationsettings.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.cpp

