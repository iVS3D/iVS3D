# Custom cmake config file by jcarius to enable find_package(Onnxruntime) without modifying LIBRARY_PATH and LD_LIBRARY_PATH
#
# This will define the following variables:
#   Onnxruntime_VERSION      -- The version number of the library
#   Onnxruntime_VERSION_MAJOR-- The major version number of the library
#   Onnxruntime_VERSION_MINOR-- The minor version number of the library
#   Onnxruntime_VERSION_PATCH-- The patch version number of the library
#   Onnxruntime_FOUND        -- True if the system has the Onnxruntime library
#   Onnxruntime_INCLUDE_DIRS -- The include directories for Onnxruntime
#   Onnxruntime_LIBRARIES    -- Libraries to link against
#   Onnxruntime_CXX_FLAGS    -- Additional (required) compiler flags

# To work correctly, you must configure Onnxruntime_ROOT_DIR to point to the root folder of Onnxruntime.
# The folder structure should be:
#   <Onnxruntime_ROOT_DIR>
#       ├── include
#       │   └── onnxruntime_cxx_api.h
#       ├── lib
#       │   └── onnxruntime.so

if(Onnxruntime_FOUND OR Onnxruntime_ALREADY_INCLUDED)
    return()
endif()

set(Onnxruntime_ALREADY_INCLUDED TRUE)

include(FindPackageHandleStandardArgs)

# Require the user to set Onnxruntime_ROOT_DIR
if(NOT Onnxruntime_ROOT_DIR)
    message(STATUS "Onnxruntime_ROOT_DIR is not set. Please set Onnxruntime_ROOT_DIR to the ONNX Runtime installation directory.")
    set(Onnxruntime_FOUND FALSE)
    return()
endif()

set(Onnxruntime_INCLUDE_DIRS 
    "${Onnxruntime_ROOT_DIR}/include" 
    "${Onnxruntime_ROOT_DIR}/include/core/providers" 
    "${Onnxruntime_ROOT_DIR}/include/core/providers/cuda")

set(Onnxruntime_LIBRARIES onnxruntime.lib)
set(Onnxruntime_CXX_FLAGS "") # no flags needed

find_library(Onnxruntime_LIBRARY onnxruntime
    PATHS "${Onnxruntime_ROOT_DIR}/lib"
)
# Only set the version if we have a valid library
if(Onnxruntime_LIBRARY)
    set(Onnxruntime_VERSION 1.22.0)
    set(Onnxruntime_VERSION_MAJOR 1)
    set(Onnxruntime_VERSION_MINOR 22)
    set(Onnxruntime_VERSION_PATCH 0)

    add_library(Onnxruntime SHARED IMPORTED)
    set_property(TARGET Onnxruntime PROPERTY IMPORTED_LOCATION "${Onnxruntime_LIBRARY}")
    set_property(TARGET Onnxruntime PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Onnxruntime_INCLUDE_DIRS}")
    set_property(TARGET Onnxruntime PROPERTY INTERFACE_COMPILE_OPTIONS "${Onnxruntime_CXX_FLAGS}")
    set_property(TARGET Onnxruntime PROPERTY IMPORTED_IMPLIB "${Onnxruntime_LIBRARY}")
endif()

find_package_handle_standard_args(Onnxruntime 
    REQUIRED_VARS Onnxruntime_LIBRARY Onnxruntime_INCLUDE_DIRS 
    HANDLE_COMPONENTS)