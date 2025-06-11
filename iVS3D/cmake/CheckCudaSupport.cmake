if(DEFINED HAS_CUDA)
    message(STATUS "WITH_CUDA is already defined as ${WITH_CUDA}. Skipping CUDA checks.")
    return()
endif()


include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    set(HAS_CUDA TRUE)
else()
    set(HAS_CUDA FALSE)
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

# Set WITH_CUDA define if both are present
if (HAS_CUDA AND HAS_OPENCV_CUDA)
    set(WITH_CUDA TRUE CACHE BOOL "Enable CUDA acceleration via OpenCV" FORCE)
    message(STATUS "CUDA and OpenCV with CUDA found. WITH_CUDA enabled.")
else()
    set(WITH_CUDA FALSE CACHE BOOL "Enable CUDA acceleration via OpenCV" FORCE)
    message(STATUS "CUDA or OpenCV with CUDA not found. WITH_CUDA disabled.")
endif()