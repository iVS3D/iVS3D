
# Define a helper function
function(add_example name)
    add_executable(${name} ${name}.cpp)

    target_link_libraries(${name}
        PUBLIC NeuralNet
    )

    install(TARGETS ${name}
        DESTINATION bin/examples
    )

    message(STATUS "Added example: ${name}")
endfunction()

# Adding all avalibale examples
add_example(map)
add_example(reduce)
add_example(reduceWithIndex)

add_component_summary("${TARGET_NAME}-Examples" TRUE)