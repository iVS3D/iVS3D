# CMakeLists.txt for iVS3D-neuralNet tests

set(TARGET_NAME "iVS3D-neuralNet-tests")

# Find the QtTest module
find_package(QT NAMES Qt5 REQUIRED COMPONENTS Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

enable_testing()

# Include directories for the test
find_package(OpenCV 4.7 REQUIRED COMPONENTS
    core
    imgproc
    video
    dnn
)

# Create a test executable
add_executable(${TARGET_NAME}
    TensorConversion.cpp
)

# Add the test to CTest
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

if(WIN32)
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

    if(DEFINED Onnxruntime_ROOT_DIR)
        list(APPEND _env_mod "PATH=path_list_prepend:${Onnxruntime_ROOT_DIR}/bin")
        list(APPEND _env_mod "PATH=path_list_prepend:${Onnxruntime_ROOT_DIR}/lib")
    endif()

    if(_env_mod)
        set_tests_properties(${TARGET_NAME} PROPERTIES ENVIRONMENT_MODIFICATION "${_env_mod}")
    endif()
endif()

# Link the QtTest module
target_link_libraries(${TARGET_NAME}
    Qt${QT_VERSION_MAJOR}::Test
    ${OpenCV_LIBRARIES}
    NeuralNet
)


