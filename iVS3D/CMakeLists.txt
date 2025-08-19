# Add tl::expected (header-only)
add_library(tl-expected INTERFACE)
target_include_directories(tl-expected INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tl-expected/include
)

# Add the project sources such as core, plugins and neuralnet
add_subdirectory(src/)

# add 'tests' as subdirectory if corresponding option is set to 'On'
if(Build_Tests)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Test)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

    enable_testing()

    add_subdirectory(tests/)
    add_component_summary("iVS3D-Tests" TRUE)
endif() # Build_Tests