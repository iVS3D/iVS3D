QT       += core gui concurrent positioning

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += console
win32:QMAKE_CXXFLAGS += /std:c++17

RC_ICONS = resources/ivs3dIcon.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

src_files = $$files(*.cpp, true)
for(file, src_files) {
    src_dirs += $$dirname(file)
}
INCLUDEPATH += $$unique(src_dirs)
INCLUDEPATH += $$_PRO_FILE_PWD_
SOURCES += $$files(*.cpp, true) \
    view/historyitem.cpp
FORMS += $$files(*.ui, true) \
    ots/colmapwrapper/colmapqueueitem_failed.ui \
    view/historyitem.ui
HEADERS += $$files(*.h, true) \
    view/historyitem.h

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/release -liVS3D-pluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/debug -liVS3D-pluginInterface
else:unix: LIBS += -L$$OUT_PWD -liVS3D-pluginInterface

INCLUDEPATH += $$PWD/../iVS3D-pluginInterface
DEPENDPATH  += $$PWD/../iVS3D-pluginInterface

# Include opencv using a local 3rdparty.pri
!include(../../3rdparty.pri){
	message(FAILED TO FIND 3rdparty.pri)
}

DISTFILES += \
    ots/ColmapWorkerScript/ColmapWorker.py \
    ots/ColmapWorkerScript/GpsEntry.py \
    ots/ColmapWorkerScript/exifread/__init__.py \
    ots/ColmapWorkerScript/exifread/classes.py \
    ots/ColmapWorkerScript/exifread/exceptions.py \
    ots/ColmapWorkerScript/exifread/exif_log.py \
    ots/ColmapWorkerScript/exifread/heic.py \
    ots/ColmapWorkerScript/exifread/jpeg.py \
    ots/ColmapWorkerScript/exifread/tags/__init__.py \
    ots/ColmapWorkerScript/exifread/tags/exif.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/__init__.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/apple.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/canon.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/casio.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/fujifilm.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/nikon.py \
    ots/ColmapWorkerScript/exifread/tags/makernote/olympus.py \
    ots/ColmapWorkerScript/exifread/utils.py \
    ots/ColmapWorkerScript/pymap3d/__init__.py \   
    ots/ColmapWorkerScript/pymap3d/aer.py \
    ots/ColmapWorkerScript/pymap3d/azelradec.py \
    ots/ColmapWorkerScript/pymap3d/ecef.py \
    ots/ColmapWorkerScript/pymap3d/eci.py \
    ots/ColmapWorkerScript/pymap3d/ellipsoid.py \
    ots/ColmapWorkerScript/pymap3d/enu.py \
    ots/ColmapWorkerScript/pymap3d/haversine.py \
    ots/ColmapWorkerScript/pymap3d/latitude.py \
    ots/ColmapWorkerScript/pymap3d/los.py \
    ots/ColmapWorkerScript/pymap3d/lox.py \
    ots/ColmapWorkerScript/pymap3d/mathfun.py \
    ots/ColmapWorkerScript/pymap3d/ned.py \
    ots/ColmapWorkerScript/pymap3d/rcurve.py \
    ots/ColmapWorkerScript/pymap3d/rsphere.py \
    ots/ColmapWorkerScript/pymap3d/sidereal.py \
    ots/ColmapWorkerScript/pymap3d/spherical.py \
    ots/ColmapWorkerScript/pymap3d/tests/__init__.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_aer.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_eci.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_elliposid.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_enu.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_geodetic.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_latitude.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_look_spheroid.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_ned.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_pyproj.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_rcurve.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_rhumb.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_rsphere.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_sidereal.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_sky.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_spherical.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_time.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_vincenty.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_vincenty_dist.py \
    ots/ColmapWorkerScript/pymap3d/tests/test_vincenty_vreckon.py \
    ots/ColmapWorkerScript/pymap3d/timeconv.py \
    ots/ColmapWorkerScript/pymap3d/utils.py \
    ots/ColmapWorkerScript/pymap3d/vallado.py \
    ots/ColmapWorkerScript/pymap3d/vdist/__init__.py \
    ots/ColmapWorkerScript/pymap3d/vdist/__main__.py \
    ots/ColmapWorkerScript/pymap3d/vincenty.py \
    ots/ColmapWorkerScript/pymap3d/vreckon/__init__.py \
    ots/ColmapWorkerScript/pymap3d/vreckon/__main__.py \
    ots/ColmapWorkerScript/yaml/__init__.py \   
    ots/ColmapWorkerScript/yaml/composer.py \
    ots/ColmapWorkerScript/yaml/constructor.py \
    ots/ColmapWorkerScript/yaml/cyaml.py \
    ots/ColmapWorkerScript/yaml/dumper.py \
    ots/ColmapWorkerScript/yaml/emitter.py \
    ots/ColmapWorkerScript/yaml/error.py \
    ots/ColmapWorkerScript/yaml/events.py \
    ots/ColmapWorkerScript/yaml/loader.py \
    ots/ColmapWorkerScript/yaml/nodes.py \
    ots/ColmapWorkerScript/yaml/parser.py \
    ots/ColmapWorkerScript/yaml/reader.py \
    ots/ColmapWorkerScript/yaml/representer.py \
    ots/ColmapWorkerScript/yaml/resolver.py \
    ots/ColmapWorkerScript/yaml/scanner.py \
    ots/ColmapWorkerScript/yaml/serializer.py \
    ots/ColmapWorkerScript/yaml/tokens.py

RESOURCES += \
    darkstyle.qrc \
    resources.qrc

unix {
    RESOURCES += lib3D_ots.qrc
    include(prebuild_cmds.pri)
}

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

unix:system(lrelease $$PWD/translations/*.ts)

OTHER_FILES += \
    copy_to_install_dir/colmap/* \

win32 {
    INCLUDEPATH -= ots \
                   ots/colmapwrapper
    SOURCES -=  ots/colmapwrapper/colmapnewproductdialog.cpp \
                ots/colmapwrapper/colmapqueueitem.cpp \
                ots/colmapwrapper/colmapsettingsdialog.cpp \
                ots/colmapwrapper/colmapviewwidget.cpp \
                ots/colmapwrapper.cpp \
                ots/translations.cpp

    HEADERS -=  ots/colmapwrapper/colmapnewproductdialog.h \
                ots/colmapwrapper/colmapqueueitem.h \
                ots/colmapwrapper/colmapsettingsdialog.h \
                ots/colmapwrapper/colmapviewwidget.h \
                ots/colmapwrapper.h \
                ots/translations.h \
                ots/global.h
}

