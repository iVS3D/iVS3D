
add_library(neuralnet STATIC
    src/Tensor.cpp
    src/OrtNeuralNet.cpp
    src/NeuralNetFactory.cpp
)

find_package(OpenCV 4.7 REQUIRED COMPONENTS
    core
    imgproc
    video
    dnn
)

include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})

target_include_directories(neuralnet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(neuralnet
    PUBLIC ${OpenCV_LIBRARIES} onnxruntime
)