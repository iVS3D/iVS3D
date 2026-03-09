
set(TEST_RESOURCES_PATH 
    "${CMAKE_CURRENT_LIST_DIR}/testresources"
)

set(IVS_SRC_PATH 
    "${CMAKE_CURRENT_LIST_DIR}/../src"
)

file(GLOB_RECURSE CORE_HEADER_FILES
    "${IVS_SRC_PATH}/iVS3D-core/*.h"
)
file(GLOB_RECURSE PLUGING_HEADER_FILES
    "${IVS_SRC_PATH}/iVS3D-pluginInterface/*.h"
)
set(RESOURCE_HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/resourceloader.h"
)

file(GLOB_RECURSE CORE_CONTROLLER_FILES
    "${IVS_SRC_PATH}/iVS3D-core/controller/*.cpp"
)
file(GLOB_RECURSE CORE_VIEW_FILES
    "${IVS_SRC_PATH}/iVS3D-core/view/*.cpp"
)
file(GLOB_RECURSE CORE_MODEL_FILES
    "${IVS_SRC_PATH}/iVS3D-core/model/*.cpp"
)
file(GLOB_RECURSE CORE_OTS_FILES
    "${IVS_SRC_PATH}/iVS3D-core/ots/*.cpp"
)
file(GLOB_RECURSE CORE_DARKSTYLE_FILES
    "${IVS_SRC_PATH}/iVS3D-core/darkstyle/*.cpp"
)
file(GLOB_RECURSE CORE_PLUGIN_FILES
    "${IVS_SRC_PATH}/iVS3D-core/plugin/*.cpp"
)
set(CORE_SRC_FILES 
    "${CORE_CONTROLLER_FILES}"
    "${CORE_VIEW_FILES}"
    "${CORE_MODEL_FILES}"
    "${CORE_OTS_FILES}"
    "${CORE_DARKSTYLE_FILES}"
    "${CORE_PLUGIN_FILES}"
    "${IVS_SRC_PATH}/iVS3D-core/applicationsettings.cpp"
    "${IVS_SRC_PATH}/iVS3D-core/stringcontainer.cpp"
)

file(GLOB_RECURSE PLUGING_SRC_FILES
    "${IVS_SRC_PATH}/iVS3D-pluginInterface/*.cpp"
)

# file(GLOB_RECURSE CORE_UI_FILES
#     "${IVS_SRC_PATH}/iVS3D-core/*.ui"
# )

# populate include directories from header files
foreach(file ${CORE_HEADER_FILES})
    get_filename_component(file_dir ${file} DIRECTORY)
    include_directories(${file_dir})
endforeach()
foreach(file ${PLUGING_HEADER_FILES})
    get_filename_component(file_dir ${file} DIRECTORY)
    include_directories(${file_dir})
endforeach()
foreach(file ${RESOURCE_HEADER_FILES})
    get_filename_component(file_dir ${file} DIRECTORY)
    include_directories(${file_dir})
endforeach()

# define costom target to download resource files
if(WIN32)
    add_custom_target(PullResources-${TARGET_NAME}
        COMMAND cmd /c "${CMAKE_CURRENT_LIST_DIR}/drf.bat"
    )
else()
    add_custom_target(PullResources-${TARGET_NAME}
        COMMAND bash "${CMAKE_CURRENT_LIST_DIR}/drf.sh"
    )
endif()

function(ivs3d_configure_test_runtime test_name)
    if(NOT WIN32)
        return()
    endif()

    set(_env_mod)

    get_target_property(_qtcore_loc_dbg Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_DEBUG)
    get_target_property(_qtcore_loc_rel Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_RELEASE)
    get_target_property(_qtcore_loc_gen Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION)

    if(_qtcore_loc_dbg)
        get_filename_component(_qt_bin "${_qtcore_loc_dbg}" DIRECTORY)
        list(APPEND _env_mod "PATH=path_list_prepend:${_qt_bin}")
    elseif(_qtcore_loc_rel)
        get_filename_component(_qt_bin "${_qtcore_loc_rel}" DIRECTORY)
        list(APPEND _env_mod "PATH=path_list_prepend:${_qt_bin}")
    elseif(_qtcore_loc_gen)
        get_filename_component(_qt_bin "${_qtcore_loc_gen}" DIRECTORY)
        list(APPEND _env_mod "PATH=path_list_prepend:${_qt_bin}")
    endif()

    if(DEFINED OpenCV_DIR)
        list(APPEND _env_mod "PATH=path_list_prepend:${OpenCV_DIR}/x64/vc17/bin")
    endif()

    if(DEFINED Ffmpeg_ROOT_DIR)
        list(APPEND _env_mod "PATH=path_list_prepend:${Ffmpeg_ROOT_DIR}/bin")
    endif()

    if(DEFINED Onnxruntime_ROOT_DIR)
        list(APPEND _env_mod "PATH=path_list_prepend:${Onnxruntime_ROOT_DIR}/bin")
        list(APPEND _env_mod "PATH=path_list_prepend:${Onnxruntime_ROOT_DIR}/lib")
    endif()

    if(_env_mod)
        set_tests_properties(${test_name} PROPERTIES ENVIRONMENT_MODIFICATION "${_env_mod}")
    endif()
endfunction()