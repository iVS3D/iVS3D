
# Define a helper function
function(add_example name)
    set(options USE_CXXOPTS USE_HELPERS)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(SOURCES ${name}.cpp)
    if(ARG_USE_CXXOPTS)
        list(APPEND SOURCES cxxopts.hpp)
    endif()

    if(ARG_USE_HELPERS)
        list(APPEND SOURCES helpers.hpp)
    endif()

    add_executable(${name} ${SOURCES})

    target_link_libraries(${name}
        PUBLIC NeuralNet
    )

    install(TARGETS ${name}
        DESTINATION bin/examples
    )

    message(STATUS "Added example: ${name}")
endfunction()

# Adding all available examples
add_example(map)
add_example(reduce)
add_example(reduceWithIndex)
add_example(inference_segmentation USE_CXXOPTS USE_HELPERS)

add_component_summary("${TARGET_NAME}-Examples" TRUE)