set(TARGET_NAME "NeuralNet")

if(NOT Onnxruntime_FOUND)
    message(WARNING "ONNXRuntime not found — skipping build of 'iVS3D-NeuralNet' library.")
    add_component_summary("${TARGET_NAME}" FALSE "Missing ONNX Runtime")
    return()
endif()


add_library(${TARGET_NAME} STATIC
    src/Tensor.cpp
    src/OrtNeuralNet.cpp
    src/NeuralNetFactory.cpp
    src/NeuralUtil.cpp
)

find_package(OpenCV 4.7 REQUIRED COMPONENTS
    core
    imgproc
    video
    dnn
)

include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${TARGET_NAME}
    PUBLIC ${OpenCV_LIBRARIES} Onnxruntime
)

# only build examples if Build_Examples is enabled
if(Build_Examples)
    add_subdirectory(examples)
endif()

# add 'tests' as subdirectory if enabled
if(Build_Tests)
    add_subdirectory(tests/)
endif()

add_component_summary("${TARGET_NAME}" TRUE)