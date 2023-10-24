set(TARGET_NAME iVS3D-pluginInterface)

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/reader"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/metaData"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/log"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/plugin"
)

# set header files
set(PUBLIC_HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/iVS3D-pluginInterface_global.h"
    "${CMAKE_CURRENT_LIST_DIR}/ialgorithm.h"
    "${CMAKE_CURRENT_LIST_DIR}/itransform.h"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/cvmat_qmetadata.h"
)

# define library
add_library(${TARGET_NAME}  SHARED
    ${PUBLIC_HEADER_FILES}
)

# set rpath
set_target_properties(${TARGET_NAME} PROPERTIES
                      INSTALL_RPATH "$ORIGIN"
)

# install
install(TARGETS ${TARGET_NAME} 
    DESTINATION bin/plugins
)