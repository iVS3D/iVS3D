set(TARGET_NAME "ModelManagerTest")

# Enable testing
enable_testing()

# Find Qt and QtTest
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

# Add the test executable
add_executable(${TARGET_NAME}
    ModelManagerTest.cpp
)

# Define the test data directory path for the test
target_compile_definitions(${TARGET_NAME} PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata"
)

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link against necessary libraries
target_link_libraries(${TARGET_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    ModelConfig
    tl-expected
)

# Add the test to CTest
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
