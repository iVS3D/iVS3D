set(TARGET_NAME "ModelConfig")

add_library(${TARGET_NAME} STATIC
    src/ModelConfig.cpp
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

target_link_libraries(${TARGET_NAME}
    PUBLIC tl-expected Qt${QT_VERSION_MAJOR}::Core
)

# only build examples if Build_Examples is enabled
if(Build_Examples)
    add_subdirectory(examples)
endif()

# add 'tests' as subdirectory if enabled
if(Build_Tests)
    add_subdirectory(tests)
endif()

add_component_summary("${TARGET_NAME}" TRUE)