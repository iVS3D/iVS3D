set(TARGET_NAME GeoDistance)

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Quick_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Positioning_INCLUDE_DIRS}
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
    "${CMAKE_CURRENT_LIST_DIR}/geodistance.h"
)

# set source files
set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/geodistance.cpp"
)

# define target to build translation files
file(GLOB_RECURSE TRANSLATION_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/*.ts"
)
add_custom_target(BuildTranslations-${TARGET_NAME}
    COMMAND lrelease ${TRANSLATION_FILES}
)

# add ressources
qt5_add_resources(QRC_SRC_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/geodistance.qrc"
)

# define library
add_library(${TARGET_NAME} SHARED
    ${HEADER_FILES}
    ${SRC_FILES}
    ${QRC_SRC_FILES}
)
target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::Quick
    ${OpenCV_LIBRARIES} 
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
    BuildTranslations-${TARGET_NAME}
)

# install
install(TARGETS ${TARGET_NAME} 
    DESTINATION bin/plugins
)

if(Update_Translations)
    add_custom_target(UpdateTranslations-${TARGET_NAME}
        COMMAND lupdate ${CMAKE_CURRENT_LIST_DIR} -ts ${TRANSLATION_FILES} -no-obsolete
    )

    add_dependencies(${TARGET_NAME}
        UpdateTranslations-${TARGET_NAME}
    )

    add_dependencies(BuildTranslations-${TARGET_NAME}
        UpdateTranslations-${TARGET_NAME}
    )
endif()

# TODO: Windows Build
#   - Deploy DLLs
#   - Check if lrelease-pro is needed
