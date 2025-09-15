# Custom cmake config file to enable find_package(Ffmpeg) for both Linux and Windows
#
# This will define the following variables:
#   Ffmpeg_FOUND           -- True if the system has the Ffmpeg libraries
#   Ffmpeg_INCLUDE_DIRS    -- The include directories for Ffmpeg
#   Ffmpeg_LIBRARIES       -- Libraries to link against
#   Ffmpeg_VERSION         -- The version number of the library (if available)
#
# On Linux, uses pkg-config to find libraries.
# On Windows, expects Ffmpeg_ROOT_DIR to be set to the root folder of Ffmpeg with include/ and lib/ subfolders.

if(Ffmpeg_FOUND OR Ffmpeg_ALREADY_INCLUDED)
    return()
endif()
set(Ffmpeg_ALREADY_INCLUDED TRUE)

include(FindPackageHandleStandardArgs)

if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVFORMAT REQUIRED libavformat)
    pkg_check_modules(AVCODEC REQUIRED libavcodec)
    pkg_check_modules(AVUTIL REQUIRED libavutil)
    pkg_check_modules(SWSCALE REQUIRED libswscale)

    set(Ffmpeg_INCLUDE_DIRS ${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})
    set(Ffmpeg_LIBRARIES ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})
    set(Ffmpeg_VERSION ${AVFORMAT_VERSION})
else()
    # Windows: require Ffmpeg_ROOT_DIR
    if(NOT Ffmpeg_ROOT_DIR)
        message(FATAL_ERROR "Ffmpeg_ROOT_DIR is not set. Please set Ffmpeg_ROOT_DIR to the FFMPEG installation directory. Ffmpeg is required.")
        set(Ffmpeg_FOUND FALSE)
        return()
    endif()

    set(Ffmpeg_INCLUDE_DIRS "${Ffmpeg_ROOT_DIR}/include")
    set(Ffmpeg_LIBRARIES
        "${Ffmpeg_ROOT_DIR}/lib/avformat.lib"
        "${Ffmpeg_ROOT_DIR}/lib/avcodec.lib"
        "${Ffmpeg_ROOT_DIR}/lib/avutil.lib"
        "${Ffmpeg_ROOT_DIR}/lib/swscale.lib"
    )

    # Helper macro to extract major version from a header


    macro(_ffmpeg_extract_major_version header macro_name var_name)
        if(EXISTS "${header}")
            file(STRINGS "${header}" _ver_line REGEX "#define[ \t]+${macro_name}[ \t]+[0-9]+")
            string(REGEX REPLACE ".*#define[ \t]+${macro_name}[ \t]+([0-9]+).*" "\\1" ${var_name} "${_ver_line}")
        else()
            set(${var_name} "missing")
        endif()
    endmacro()


    string(REPLACE "\\" "/" _ffmpeg_avformat_header "${Ffmpeg_ROOT_DIR}/include/libavformat/version_major.h")
    string(REPLACE "\\" "/" _ffmpeg_avcodec_header "${Ffmpeg_ROOT_DIR}/include/libavcodec/version_major.h")
    string(REPLACE "\\" "/" _ffmpeg_avutil_header "${Ffmpeg_ROOT_DIR}/include/libavutil/version.h")
    string(REPLACE "\\" "/" _ffmpeg_swscale_header "${Ffmpeg_ROOT_DIR}/include/libswscale/version_major.h")

    _ffmpeg_extract_major_version("${_ffmpeg_avformat_header}" "LIBAVFORMAT_VERSION_MAJOR" Ffmpeg_AVFORMAT_VERSION_MAJOR)
    _ffmpeg_extract_major_version("${_ffmpeg_avcodec_header}" "LIBAVCODEC_VERSION_MAJOR" Ffmpeg_AVCODEC_VERSION_MAJOR)
    _ffmpeg_extract_major_version("${_ffmpeg_avutil_header}" "LIBAVUTIL_VERSION_MAJOR" Ffmpeg_AVUTIL_VERSION_MAJOR)
    _ffmpeg_extract_major_version("${_ffmpeg_swscale_header}" "LIBSWSCALE_VERSION_MAJOR" Ffmpeg_SWSCALE_VERSION_MAJOR)

    set(Ffmpeg_VERSION ${Ffmpeg_AVFORMAT_VERSION_MAJOR})
    set(Ffmpeg_SUBLIB_VERSIONS
        "avformat:${Ffmpeg_AVFORMAT_VERSION_MAJOR}"
        "avcodec:${Ffmpeg_AVCODEC_VERSION_MAJOR}"
        "avutil:${Ffmpeg_AVUTIL_VERSION_MAJOR}"
        "swscale:${Ffmpeg_SWSCALE_VERSION_MAJOR}"
    )
endif()

find_package_handle_standard_args(Ffmpeg
    REQUIRED_VARS Ffmpeg_INCLUDE_DIRS Ffmpeg_LIBRARIES
    VERSION_VAR Ffmpeg_VERSION
)
