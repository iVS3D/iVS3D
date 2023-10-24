set(TARGET_NAME CameraMovement)

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

# CUDA (if applicable)
if(With_CUDA)

    # cudaoptflow from opencv is required
    find_package(OpenCV 4.5 REQUIRED COMPONENTS
        cudaoptflow
    )

    include_directories(SYSTEM
        ${CUDA_INCLUDE_DIRS}
        ${CUDNN_INCLUDE_DIR}
    )

    set(CUDA_HEADER_FILES
        "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowgpu.h"
    )

    set(CUDA_SRC_FILES
        "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowgpu.cpp"
    )
endif()

# set header files
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/cameramovement.h"
    "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflow.h"
    "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowcpu.h"
    "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowfactory.h"
    ${CUDA_HEADER_FILES}
)

# set source files
set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/cameramovement.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowcpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/farnebackoptflowfactory.cpp"
    ${CUDA_SRC_FILES}
)

# define library
add_library(${TARGET_NAME} SHARED
    ${HEADER_FILES}
    ${SRC_FILES}
)
target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Positioning
    ${OpenCV_LIBRARIES} 
    ${CUDA_LIBRARIES}
    ${CUDNN_LIBRARIES}
)

# add defines
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
)

# set rpath
set_target_properties(${TARGET_NAME} PROPERTIES
                      INSTALL_RPATH "$ORIGIN"
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

# TODO: Windows Build
#   - Deploy DLLs
#   - Check if lrelease-pro is needed