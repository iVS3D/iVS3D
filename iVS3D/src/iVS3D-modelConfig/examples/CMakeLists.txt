# ObjectDetection Examples

# Enable Qt MOC, UIC, and RCC for all targets in this directory
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Usage Example - demonstrates how to use ModelSettingsWidget
add_executable(ModelSettingsWidget_Example
    gui_example.cpp
)

target_link_libraries(ModelSettingsWidget_Example
    PRIVATE
    ModelConfig
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    tl-expected
)