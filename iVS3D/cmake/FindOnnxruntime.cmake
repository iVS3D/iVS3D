# Custom cmake config file by jcarius to enable find_package(onnxruntime) without modifying LIBRARY_PATH and LD_LIBRARY_PATH
#
# This will define the following variables:
#   onnxruntime_VERSION      -- The version number of the library
#   onnxruntime_VERSION_MAJOR-- The major version number of the library
#   onnxruntime_VERSION_MINOR-- The minor version number of the library
#   onnxruntime_VERSION_PATCH-- The patch version number of the library
#   onnxruntime_FOUND        -- True if the system has the onnxruntime library
#   onnxruntime_INCLUDE_DIRS -- The include directories for onnxruntime
#   onnxruntime_LIBRARIES    -- Libraries to link against
#   onnxruntime_CXX_FLAGS    -- Additional (required) compiler flags

# To work correctly, you must configure onnxruntime_INSTALL_PREFIX to point to the installation prefix of onnxruntime.
# The folder structure should be:
#   <install-prefix>
#       ├── include
#       │   └── onnxruntime_cxx_api.h
#       ├── lib
#       │   └── onnxruntime.so

set(onnxruntime_VERSION 1.22.0)
set(onnxruntime_VERSION_MAJOR 1)
set(onnxruntime_VERSION_MINOR 22)
set(onnxruntime_VERSION_PATCH 0)

include(FindPackageHandleStandardArgs)

set(onnxruntime_INCLUDE_DIRS "${onnxruntime_INSTALL_PREFIX}/include" "${onnxruntime_INSTALL_PREFIX}/include/core/providers" "${onnxruntime_INSTALL_PREFIX}/include/core/providers/cuda")
set(onnxruntime_LIBRARIES onnxruntime.lib)
set(onnxruntime_CXX_FLAGS "") # no flags needed



find_library(onnxruntime_LIBRARY onnxruntime
    PATHS "${onnxruntime_INSTALL_PREFIX}/lib"
)

add_library(onnxruntime SHARED IMPORTED)
set_property(TARGET onnxruntime PROPERTY IMPORTED_LOCATION "${onnxruntime_LIBRARY}")
set_property(TARGET onnxruntime PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_INCLUDE_DIRS}")
set_property(TARGET onnxruntime PROPERTY INTERFACE_COMPILE_OPTIONS "${onnxruntime_CXX_FLAGS}")

set_property(TARGET onnxruntime PROPERTY IMPORTED_IMPLIB "${onnxruntime_LIBRARY}")

find_package_handle_standard_args(onnxruntime 
    REQUIRED_VARS onnxruntime_LIBRARY onnxruntime_INCLUDE_DIRS 
    VERSION_VAR onnxruntime_VERSION)