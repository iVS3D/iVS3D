set(TARGET_NAME 0_NthFrame)

# Set C++ standard for this target
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define header files
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/nthframe.h"
)

# Define source files
set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/nthframe.cpp"
)

# Define translation files
set(TS_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/nth_de.ts"
    "${CMAKE_CURRENT_LIST_DIR}/translations/nth_en.ts"
)

set(QM_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/nth_de.qm"
    "${CMAKE_CURRENT_LIST_DIR}/translations/nth_en.qm"
)

# Generate compiled translation files (.qm) from source translations (.ts)
add_custom_command(
    OUTPUT ${QM_FILES}
    COMMAND lrelease ${TS_FILES}
    DEPENDS ${TS_FILES}
)

# Process resource files
qt5_add_resources(QRC_SRC_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/resources.qrc"
)

# Create shared library plugin
add_library(${TARGET_NAME} SHARED
    ${HEADER_FILES}
    ${SRC_FILES}
    ${QRC_SRC_FILES}
    ${QM_FILES}
    ${TS_FILES}
)

# Link against required libraries
target_link_libraries(${TARGET_NAME}
    PRIVATE
        iVS3D-pluginInterface
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Concurrent
        ${OpenCV_LIBRARIES}
        tl-expected
)

# Add system include directories
target_include_directories(${TARGET_NAME}
    SYSTEM PRIVATE
        ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
        ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
        ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
)

# Add include directories
target_include_directories(${TARGET_NAME}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-pluginInterface"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/reader"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/metaData"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/log"
        "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/plugin"
)

# Add compile definitions
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
)

# Set properties
set_target_properties(${TARGET_NAME}
    PROPERTIES
        INSTALL_RPATH "$ORIGIN"
)

# Add dependency to iVS3D-pluginInterface
add_dependencies(${TARGET_NAME} 
    iVS3D-pluginInterface
)

# Install plugin
install(TARGETS ${TARGET_NAME} 
    DESTINATION bin/plugins
)

# Update translation files if requested
if(Update_Translations)
    add_custom_command(
        OUTPUT ${TS_FILES} placeholder.out
        COMMAND lupdate ${CMAKE_CURRENT_LIST_DIR} -ts ${TS_FILES} -no-obsolete
        DEPENDS ${HEADER_FILES} ${SRC_FILES}
    )
endif()

# Add plugin summary to build output
add_plugin_summary("${TARGET_NAME}" TRUE)