cmake_minimum_required(VERSION 3.14.0)

# get version from git-tag
include(cmake/gitversion.cmake)
string(TIMESTAMP TODAY "%Y-%m-%d")

# set project name
project(iVS3D VERSION ${VERSION_STR})

# define version and build date
set(COMMON_COMPILE_DEFINITIONS ${COMMON_COMPILE_DEFINITIONS}
    -DIVS3D_VER=${VERSION_STR}
    -DIVS3D_DAT=${TODAY}
)

# Set build type to release if not specified otherwise
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release | Debug" FORCE)
endif()

# Set 'Build_Plugins' option (default true)
set(Build_Plugins ON CACHE BOOL "Build plugins.")

# Set 'Install_Models' option (default true)
set(Install_Models ON CACHE BOOL "Copy neural network models to install directory.")

# Set 'Build_Tests' option (default false)
set(Build_Tests OFF CACHE BOOL "Build test routines.")

# Set 'Update_Translations' option (default false)
set(Update_Translations OFF CACHE BOOL "Enable rescanning sources to update .ts files.")

# Set 'With_CUDA' option (default true)
set(With_CUDA ON CACHE BOOL "Build with CUDA library")

# CXX Flags
set(CMAKE_CXX_STANDARD 17)
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17")
endif()

# Add paths to linker search and installed rpath.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# 3rd_party

# Qt
find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Gui Widgets Concurrent Positioning Svg Quick Qml Network Location LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Concurrent Positioning Svg Quick Qml Network Location LinguistTools)
set(CMAKE_AUTOUIC ON) # The AUTOGEN_BUILD_DIR is automatically added to the target's INCLUDE_DIRECTORIES.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# OpenCV
find_package(OpenCV 4.7 REQUIRED COMPONENTS
    core
    imgcodecs
    videoio
)

# CUDA (if applicable)
if(With_CUDA)

    find_package(CUDA 12.0 REQUIRED)

    # set CUDNN variables
    set(CUDNN_INCLUDE_DIR "${CUDA_TOOLKIT_ROOT_DIR}/include" PATH FORCE)
    set(CUDNN_LIBRARY "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libcudnn.so" FILEPATH FORCE)

    set(COMMON_COMPILE_DEFINITIONS ${COMMON_COMPILE_DEFINITIONS}
        -DWITH_CUDA
    )

endif() # With_CUDA

if(Update_Translations)
    # Qt Linguist
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS LinguistTools)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS LinguistTools)
endif()

# subdirectories

# add 'src' as subdirectory
add_subdirectory(src/)

# add 'tests' as subdirectory if corresponding optin is set to 'On'
if(Build_Tests)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Test)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

    enable_testing()

    add_subdirectory(tests/)
endif() # Build_Tests

