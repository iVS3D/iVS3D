TEST_RESOURCES_PATH = $$PWD/testresources
IVS_SRC_PATH = $$PWD/../src

QT += widgets gui concurrent

HEADERS += resourceloader.h

INCLUDEPATH += $$PWD

src_files = $$files($$IVS_SRC_PATH/iVS3D-core/*.cpp, true)
for(file, src_files) {
    src_dirs += $$dirname(file)
}
INCLUDEPATH += $$unique(src_dirs)
INCLUDEPATH += \
    $$IVS_SRC_PATH/iVS3D-pluginInterface \

DEPENDPATH  += $$IVS_SRC_PATH/iVS3D-pluginInterface


SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/controller/*.cpp, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/view/*.cpp, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/model/*.cpp, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/darkstyle/*.cpp, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/plugin/*.cpp, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-core/ots/*.cpp, true)
SOURCES += $$IVS_SRC_PATH/iVS3D-core/applicationsettings.cpp
SOURCES += $$IVS_SRC_PATH/iVS3D-core/stringcontainer.cpp
HEADERS += $$files($$IVS_SRC_PATH/iVS3D-core/*.h, true)
FORMS += $$files($$IVS_SRC_PATH/iVS3D-core/*.ui, true)
SOURCES += $$files($$IVS_SRC_PATH/iVS3D-pluginInterface/*.cpp, true)
HEADERS += $$files($$IVS_SRC_PATH/iVS3D-pluginInterface/*.h, true)
