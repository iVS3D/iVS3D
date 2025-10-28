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

# Link the QtTest module
target_link_libraries(${TARGET_NAME}
    Qt${QT_VERSION_MAJOR}::Test
    ${OpenCV_LIBRARIES}
    NeuralNet
)


