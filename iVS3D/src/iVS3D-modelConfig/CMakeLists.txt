set(TARGET_NAME "ModelConfig")

set(CMAKE_AUTOMOC ON) # Enable Qt MOC for the ModelSettingsWidget

set(MODEL_CONFIG_HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/include/ModelConfig.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/ModelManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/ModelSettingsWidget.h"
)

set(MODEL_CONFIG_SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/src/ModelConfig.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/ModelManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/ModelSettingsWidget.cpp"
)

set(QM_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/modelconfig_de.qm"
    "${CMAKE_CURRENT_LIST_DIR}/translations/modelconfig_en.qm"
)

set(TS_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/modelconfig_de.ts"
    "${CMAKE_CURRENT_LIST_DIR}/translations/modelconfig_en.ts"
)

add_custom_command(
    OUTPUT ${QM_FILES}
    COMMAND lrelease ${TS_FILES}
    DEPENDS ${TS_FILES}
)

qt5_add_resources(QRC_SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/modelconfig_translations.qrc"
)

add_library(${TARGET_NAME} STATIC
    ${MODEL_CONFIG_SRC_FILES}
    ${MODEL_CONFIG_HEADER_FILES}
    ${QRC_SRC_FILES}
    ${QM_FILES}
    ${TS_FILES}
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME} PUBLIC 
    tl-expected 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# only build examples if Build_Examples is enabled
if(Build_Examples)
    add_subdirectory(examples)
endif()

# add 'tests' as subdirectory if enabled
if(Build_Tests)
    add_subdirectory(tests)
endif()

if(Update_Translations)
    add_custom_command(
        OUTPUT ${TS_FILES} placeholder.out
        COMMAND lupdate ${CMAKE_CURRENT_LIST_DIR} -ts ${TS_FILES} -no-obsolete
        DEPENDS ${MODEL_CONFIG_HEADER_FILES} ${MODEL_CONFIG_SRC_FILES}
    )
endif()

add_component_summary("${TARGET_NAME}" TRUE)