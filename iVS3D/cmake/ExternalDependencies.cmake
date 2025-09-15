# Qt
find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Gui Widgets Concurrent Positioning Quick Qml Network Location Svg LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Concurrent Positioning Quick Qml Network Location Svg LinguistTools)
set(CMAKE_AUTOUIC ON) # The AUTOGEN_BUILD_DIR is automatically added to the target's INCLUDE_DIRECTORIES.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Add paths to linker search and installed rpath.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)            # Use C++17
if(NOT WIN32)
    # Use -fPIC for position-independent code on non-Windows platforms
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # Enforce C++17
set(CMAKE_CXX_EXTENSIONS OFF)         # Use only standard C++ (disable compiler-specific extensions)

# OpenCV
find_package(OpenCV 4.7 REQUIRED COMPONENTS
    core
    imgcodecs
    videoio
    dnn
    imgproc
)

include(${CMAKE_CURRENT_LIST_DIR}/CheckCudaSupport.cmake)
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

# Add your cmake/ folder to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package(Onnxruntime 1.18.0)

find_package(Ffmpeg REQUIRED)