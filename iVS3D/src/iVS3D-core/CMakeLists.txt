set(TARGET_NAME iVS3D-core)

# if windows build, set icon 
if(WIN32)
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/ivs3dIcon.ico")
endif()


# set header, source and form files
file(GLOB_RECURSE HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)
file(GLOB_RECURSE UI_FILES
    "${CMAKE_CURRENT_LIST_DIR}/*.ui"
)

# remove ots files on windows
if(WIN32)
    list(FILTER HEADER_FILES EXCLUDE REGEX "ots(.)*h")
    list(FILTER SRC_FILES EXCLUDE REGEX "ots(.)*cpp")
    list(FILTER UI_FILES EXCLUDE REGEX "ots(.)*ui")
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
)
foreach(file ${SRC_FILES})
    get_filename_component(file_dir ${file} DIRECTORY)
    include_directories(${file_dir})
endforeach()

# add ressources
qt5_add_resources(QRC_SRC_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/darkstyle.qrc"
    "${CMAKE_CURRENT_LIST_DIR}/resources.qrc"
)
if (UNIX)
    qt5_add_resources(QRC_SRC_FILES 
        "${CMAKE_CURRENT_LIST_DIR}/lib3D_ots.qrc"
    )
endif()

# set compiled translation files (.qm)
set(QM_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/core_de.qm"
    "${CMAKE_CURRENT_LIST_DIR}/translations/core_en.qm"
)

# set compiled translation files (.ts)
set(TS_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/core_de.ts"
    "${CMAKE_CURRENT_LIST_DIR}/translations/core_en.ts"
)

# custom command to create translations (.qm) files using lrelease
add_custom_command(
    OUTPUT ${QM_FILES}
    COMMAND lrelease ${TS_FILES}
    DEPENDS ${TS_FILES}
)

# define executable
add_executable(${TARGET_NAME}
    ${HEADER_FILES}
    ${SRC_FILES}
    ${UI_FILES}
    ${QRC_SRC_FILES}
    ${QM_FILES}
    ${TS_FILES}
    ${app_icon_resource_windows}
)
target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    ${OpenCV_LIBRARIES} 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Positioning
)

# add defines
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
)

# set plugins to rpath
set_target_properties(${TARGET_NAME} PROPERTIES
                      INSTALL_RPATH "$ORIGIN:$ORIGIN/plugins"
)

# add dependency to iVS3D_pluginInterface
add_dependencies(${TARGET_NAME}
    iVS3D-pluginInterface
)

# install
install(TARGETS ${TARGET_NAME} 
    DESTINATION bin
)
if(WIN32)
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/copy_to_install_dir/ 
        DESTINATION bin/colmap
        FILES_MATCHING PATTERN "*"
    )
endif()

if(Update_Translations)
    # if Update_Translations flag is set to ON, we want to override existing .ts files with new Strings from the src files.
    # this is done using Qt's tool "lupdate", which searches all src files (.cpp and .h) in the given folder (in this case
    # ${CMAKE_CURRENT_LIST_DIR}) and outputs the updated .ts files
    # IMPORTANT: We want this command to execute, even if the ts files already exist, to make sure they are updated! This is
    # achieved by adding an OUTPUT (placeholder.out) which is never created and thus forces the command to run on every build.
    add_custom_command(
        OUTPUT ${TS_FILES} placeholder.out
        COMMAND lupdate ${CMAKE_CURRENT_LIST_DIR} -ts ${TS_FILES} -no-obsolete
        DEPENDS ${HEADER_FILES} ${SRC_FILES}
    )
endif()
