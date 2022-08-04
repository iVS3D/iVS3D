QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += console

RC_ICONS = resources/ivs3dIcon.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:include(prebuild_cmds.pri)

src_files = $$files(*.cpp, true)
message($$src_files)
for(file, src_files) {
    src_dirs += $$dirname(file)
}
INCLUDEPATH += $$unique(src_dirs)
INCLUDEPATH += $$_PRO_FILE_PWD_
SOURCES += $$files(*.cpp, true)
FORMS += $$files(*.ui, true)
HEADERS += $$files(*.h, true)

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/release -liVS3D-pluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/debug -liVS3D-pluginInterface
else:unix: LIBS += -L$$OUT_PWD -liVS3D-pluginInterface

INCLUDEPATH += $$PWD/../iVS3D-pluginInterface
DEPENDPATH  += $$PWD/../iVS3D-pluginInterface

# Include opencv using a local 3rdparty.pri
include(../../3rdparty.pri)

DISTFILES +=

TRANSLATIONS = \
    $$PWD/translations/lib3D_ots_de.ts \
    $$PWD/translations/lib3D_ots_en.ts

RESOURCES += \
    darkstyle.qrc \
    resources.qrc \
    lib3D_ots.qrc

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


TRANSLATIONS += \
    $$PWD/translations/core_en.ts \
    $$PWD/translations/core_de.ts

OTHER_FILES += \
    copy_to_install_dir/colmap/* \

