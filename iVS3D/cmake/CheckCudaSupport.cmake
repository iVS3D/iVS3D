# Only check once!
if(DEFINED CUDA_CHECKED)
    return()
endif()
set(CUDA_CHECKED TRUE)

# If user explicitly disabled CUDA, respect it
if(DEFINED WITH_CUDA AND NOT WITH_CUDA)
    message(STATUS "CUDA support explicitly disabled by user (WITH_CUDA=OFF).")
    return()
endif()

# Check for CUDA compiler support (currently optional as we do not compile any CUDA code)
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    set(HAS_NVCC TRUE)
    message(STATUS "CUDA compiler found: ${CMAKE_CUDA_COMPILER}")
else()
    set(HAS_NVCC FALSE)
    message(STATUS "CUDA compiler not found. HAS_NVCC set to FALSE.")
endif()

# Check for CUDAToolkit
find_package(CUDAToolkit 12.0)
if(NOT CUDAToolkit_FOUND)
    set(HAS_CUDA_TOOLKIT FALSE)
    message(STATUS "CUDAToolkit not found. HAS_CUDA_TOOLKIT set to FALSE.")
else()
    set(HAS_CUDA_TOOLKIT TRUE)
    message(STATUS "CUDAToolkit found: ${CUDAToolkit_VERSION}")
endif()

# Ensure OpenCV is already found before this script is included
if (NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV must be found before including CheckCudaSupport.cmake")
endif()

# Check if OpenCV has CUDA modules
set(HAS_OPENCV_CUDA FALSE)
foreach(CUDA_MODULE opencv_cudaarithm opencv_cudafilters opencv_cudaimgproc)
    list(FIND OpenCV_LIB_COMPONENTS "${CUDA_MODULE}" _index)
    if (NOT _index EQUAL -1)
        set(HAS_OPENCV_CUDA TRUE)
        break()
    endif()
endforeach()

# Final decision on WITH_CUDA:
# - If user explicitly set WITH_CUDA=ON, only honor it if checks pass
# - If not set, enable if CUDA and OpenCV CUDA are present

if(DEFINED WITH_CUDA AND WITH_CUDA)
    if(HAS_CUDA_TOOLKIT AND HAS_OPENCV_CUDA)
        message(STATUS "WITH_CUDA is enabled and requirements are met.")
    else()
        message(WARNING "WITH_CUDA was requested but requirements are missing. Disabling WITH_CUDA.")
        set(WITH_CUDA FALSE CACHE BOOL "Enable CUDA acceleration via OpenCV" FORCE)
    endif()
elseif(NOT DEFINED WITH_CUDA)
    if(HAS_CUDA_TOOLKIT AND HAS_OPENCV_CUDA)
        set(WITH_CUDA TRUE CACHE BOOL "Enable CUDA acceleration via OpenCV" FORCE)
        message(STATUS "WITH_CUDA was not set; enabling by auto-detection.")
    else()
        set(WITH_CUDA FALSE CACHE BOOL "Enable CUDA acceleration via OpenCV" FORCE)
        message(STATUS "WITH_CUDA was not set; disabling due to missing requirements.")
    endif()
endif()
