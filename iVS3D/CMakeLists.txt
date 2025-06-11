cmake_minimum_required(VERSION 3.22.0)

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

# Add your cmake/ folder to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# Add paths to linker search and installed rpath.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)



# Setup Summary on Top Level
if(PROJECT_IS_TOP_LEVEL)
    include(cmake/Summary.cmake)
    define_summary_property()
endif()

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
    dnn
    imgproc
)

find_package(Onnxruntime 1.18.0)


include(cmake/CheckCudaSupport.cmake)
if(WITH_CUDA)

    #find_package(CUDA 12.0 REQUIRED)
    #enable_language(CUDA)
    find_package(CUDAToolkit 12.0 REQUIRED)

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
    add_component_summary("iVS3D-Tests" TRUE)
endif() # Build_Tests

if(PROJECT_IS_TOP_LEVEL)
    print_summary()
endif()