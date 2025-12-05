set(TARGET_NAME MaskPlugin)

# Can't built if iVS3D-neuralNet is not found
if(NOT TARGET NeuralNet)
    message(WARNING "iVS3D-NeuralNet not found - skipping build of 'iVS3D-${TARGET_NAME}' plugin.")
    add_plugin_summary("${TARGET_NAME}" FALSE "Missing NeuralNet")
    return()
endif()

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-pluginInterface"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/reader"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/metaData"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/log"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/plugin"
)

find_package(OpenCV 4.10 REQUIRED COMPONENTS
    imgproc
)

set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/maskplugin.h"
)

set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/maskplugin.cpp"
)

add_library(${TARGET_NAME} SHARED
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    ${OpenCV_LIBRARIES}
    ${CUDA_LIBRARIES}
    ${CUDNN_LIBRARIES}
    NeuralNet
)

# add dependency to iVS3D_pluginInterface
add_dependencies(${TARGET_NAME}
    iVS3D-pluginInterface
    iVS3D-core
)

# install
install(TARGETS 
    ${TARGET_NAME} 
    DESTINATION bin/plugins
)

add_plugin_summary("${TARGET_NAME}" TRUE)