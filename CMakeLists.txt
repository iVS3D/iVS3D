cmake_minimum_required(VERSION 3.22.0)

# get version from git-tag
include(iVS3D/cmake/gitversion.cmake)
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

# Set 'Build_Examples' option (default false)
set(Build_Examples OFF CACHE BOOL "Build examples.")

# Set 'Update_Translations' option (default false)
set(Update_Translations OFF CACHE BOOL "Enable rescanning sources to update .ts files.")

# Setup Summary on Top Level
if(PROJECT_IS_TOP_LEVEL)
    include(iVS3D/cmake/Summary.cmake)
    define_summary_property()
endif()

# 3rd_party
include(iVS3D/cmake/ExternalDependencies.cmake)

if(Update_Translations)
    # Qt Linguist
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS LinguistTools)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS LinguistTools)
endif()

# subdirectories
add_subdirectory(iVS3D)

if(PROJECT_IS_TOP_LEVEL)
    print_summary()
endif()