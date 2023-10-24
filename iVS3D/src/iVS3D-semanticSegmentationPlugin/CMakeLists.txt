set(TARGET_NAME SemanticSegmentation)

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


# set header files
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/semanticsegmentation.h"
    "${CMAKE_CURRENT_LIST_DIR}/semanticsegmentation_global.h"
    "${CMAKE_CURRENT_LIST_DIR}/settingswidget.h"
)

# set source files
set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/settingswidget.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/semanticsegmentation.cpp"
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
)

# add defines
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
    -DIVS3DSEMANTICSEGMENTATIONPLUGIN_LIBRARY
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
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/copy_to_install_dir/models/
    DESTINATION bin/models
    FILES_MATCHING PATTERN "*"
)

# TODO: Windows Build
#   - Deploy DLLs
#   - Check if lrelease-pro is needed