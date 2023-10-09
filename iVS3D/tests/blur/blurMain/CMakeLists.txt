set(TEST_NAME main)
set(TARGET_NAME iVS3D-test-blur-${TEST_NAME})

# include cmake file in resource loader
include("${CMAKE_CURRENT_LIST_DIR}/../../resourceloader.cmake")

# set header, source and form files
set(HEADER_FILES
    ${CORE_HEADER_FILES}
    ${PLUGIN_HEADER_FILES}
    ${RESOURCE_HEADER_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/reader_stub.h"
    "${CMAKE_CURRENT_LIST_DIR}/logfile_stub.h"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blur.h"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/BlurAlgorithm.h"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blursobel.h"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blurlaplacian.h"
)
set(SRC_FILES 
    ${CORE_SRC_FILES}
    ${PLUGIN_SRC_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/reader_stub.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/logfile_stub.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tst_blurMain.cpp"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blur.cpp"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/BlurAlgorithm.cpp"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blursobel.cpp"
    "${IVS_SRC_PATH}/iVS3D-blurPlugin/blurlaplacian.cpp"
)
set(UI_FILES
    ${CORE_UI_FILES}
)

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}test_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
include_directories(
    "${IVS_SRC_PATH}/iVS3D-blurPlugin"
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
    DownloadResourceFiles-${TARGET_NAME}
)
install(TARGETS ${TARGET_NAME} 
    DESTINATION tests
)

# TODO: Windows Build
#   - Deploy DLLs
#   - Check if lrelease-pro is needed
