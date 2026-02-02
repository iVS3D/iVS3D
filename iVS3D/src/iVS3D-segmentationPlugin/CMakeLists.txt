set(TARGET_NAME SegmentationPlugin)

# Can't built if iVS3D-neuralNet is not found
if(NOT TARGET NeuralNet)
    message(WARNING "iVS3D-NeuralNet not found - skipping build of 'iVS3D-${TARGET_NAME}' plugin.")
    add_plugin_summary("${TARGET_NAME}" FALSE "Missing NeuralNet")
    return()
endif()

if(NOT TARGET ModelConfig)
    message(WARNING "iVS3D-ModelConfig not found - skipping build of 'iVS3D-${TARGET_NAME}' plugin.")
    add_plugin_summary("${TARGET_NAME}" FALSE "Missing ModelConfig")
    return()
endif()

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)


find_package(OpenCV 4.10 REQUIRED COMPONENTS
    imgproc
)

set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/segplugin.h"
)

set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/segplugin.cpp"
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
    ModelConfig
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