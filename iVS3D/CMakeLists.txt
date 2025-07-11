add_subdirectory(src/)

# add 'tests' as subdirectory if corresponding option is set to 'On'
if(Build_Tests)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Test)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

    enable_testing()

    add_subdirectory(tests/)
    add_component_summary("iVS3D-Tests" TRUE)
endif() # Build_Tests