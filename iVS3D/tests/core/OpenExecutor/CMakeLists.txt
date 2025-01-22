set(TEST_NAME open-executor)
set(TARGET_NAME iVS3D-test-core-${TEST_NAME})

# include cmake file in resource loader
include("${CMAKE_CURRENT_LIST_DIR}/../../resourceloader.cmake")

# set header, source and form files
set(HEADER_FILES
    ${CORE_HEADER_FILES}
    ${PLUGIN_HEADER_FILES}
    ${RESOURCE_HEADER_FILES}
)
set(SRC_FILES 
    ${CORE_SRC_FILES}
    ${PLUGIN_SRC_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/tst_openexecutor.cpp"
)
set(UI_FILES
    ${CORE_UI_FILES}
)

# remove ots files on windows
if(WIN32)
    list(FILTER HEADER_FILES EXCLUDE REGEX "ots(.)*h")
    list(FILTER SRC_FILES EXCLUDE REGEX "ots(.)*cpp")
    list(FILTER UI_FILES EXCLUDE REGEX "ots(.)*ui")
endif()

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}test_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

####################################################################################################
# define test executable, with linklibraries, defines, rpath, dependencies, and installs
add_executable(${TARGET_NAME}
    ${HEADER_FILES}
    ${SRC_FILES}
)
add_test(${TARGET_NAME} ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    ${OpenCV_LIBRARIES} 
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Positioning
)
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
    -DTEST_RESOURCES="${TEST_RESOURCES_PATH}"
)
set_target_properties(${TARGET_NAME} PROPERTIES
                      INSTALL_RPATH "$ORIGIN:$ORIGIN/../bin/plugins"
)
add_dependencies(${TARGET_NAME}
    iVS3D-pluginInterface
    PullResources-${TARGET_NAME} 
)
install(TARGETS ${TARGET_NAME} 
    DESTINATION tests
)

# TODO: Windows Build
#   - Deploy DLLs
#   - Check if lrelease-pro is needed
