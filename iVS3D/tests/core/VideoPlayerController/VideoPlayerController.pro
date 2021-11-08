QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

SOURCES +=  tst_videoplayercontroller.cpp

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

# files to test
INCLUDEPATH += \
    $$IVS_SRC_PATH/iVS3D-core \
    $$IVS_SRC_PATH/iVS3D-pluginInterface \



HEADERS += \
    $$IVS_SRC_PATH/iVS3D-core/model/reader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/openexecutor.h \
    $$IVS_SRC_PATH/iVS3D-core/model/DataManager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/modelalgorithm.h \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.h \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/logmanager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/concurrentreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.h \
    $$IVS_SRC_PATH/iVS3D-core/model/algorithmthread.h \
    $$IVS_SRC_PATH/iVS3D-core/model/progressable.h \
    $$IVS_SRC_PATH/iVS3D-core/model/progressdisplay.h \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.h \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.h \
    $$IVS_SRC_PATH/iVS3D-core/model/jsonEnum.h \
    $$IVS_SRC_PATH/iVS3D-core/model/logfile.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/algorithmmanager.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/IAlgorithm.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/itransformrequestdequeue.h \
    $$IVS_SRC_PATH/iVS3D-core/plugin/transformmanager.h \
    $$IVS_SRC_PATH/iVS3D-pluginInterface/itransform.h \
    $$IVS_SRC_PATH/iVS3D-pluginInterface/itransform.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/videoplayercontroller.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/ModelInputIterator.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/modelinputiteratorfactory.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/keyframeiterator.h \
    $$IVS_SRC_PATH/iVS3D-core/controller/imageiterator.h \
    $$IVS_SRC_PATH/iVS3D-core/view/videoplayer.h \
    $$IVS_SRC_PATH/iVS3D-core/view/timeline.h \
    $$IVS_SRC_PATH/iVS3D-core/view/samplingwidget.h \
    $$IVS_SRC_PATH/iVS3D-core/view/slideablelabel.h \
    $$IVS_SRC_PATH/iVS3D-core/view/timelinelabel.h \
    $$IVS_SRC_PATH/iVS3D-core/view/reallydeletedialog.h \


SOURCES += \
    $$IVS_SRC_PATH/iVS3D-core/model/videoreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/imagereader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/openexecutor.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/DataManager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/modelalgorithm.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/modelinputpictures.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/logmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/concurrentreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/projectmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/algorithmthread.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/progressable.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/progressdisplay.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/stringcontainer.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/delayedcopyreader.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/jsonEnum.cpp \
    $$IVS_SRC_PATH/iVS3D-core/model/logfile.cpp \
    $$IVS_SRC_PATH/iVS3D-core/plugin/algorithmmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/plugin/itransformrequestdequeue.cpp \
    $$IVS_SRC_PATH/iVS3D-core/plugin/transformmanager.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/videoplayercontroller.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/modelinputiteratorfactory.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/keyframeiterator.cpp \
    $$IVS_SRC_PATH/iVS3D-core/controller/imageiterator.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/videoplayer.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/timeline.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/samplingwidget.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/slideablelabel.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/timelinelabel.cpp \
    $$IVS_SRC_PATH/iVS3D-core/view/reallydeletedialog.cpp \

FORMS += \
    $$IVS_SRC_PATH/iVS3D-core/view/videoplayer.ui \
    $$IVS_SRC_PATH/iVS3D-core/view/timeline.ui \
    $$IVS_SRC_PATH/iVS3D-core/view/samplingwidget.ui \
    $$IVS_SRC_PATH/iVS3D-core/view/reallydeletedialog.ui \
