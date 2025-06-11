function(define_summary_property)
    define_property(GLOBAL PROPERTY PLUGIN_SUMMARY_LIST
        BRIEF_DOCS "Plugin summary list"
        FULL_DOCS "List of plugin entries (name, status, reason)"
    )
    set_property(GLOBAL PROPERTY PLUGIN_SUMMARY_LIST "")

    define_property(GLOBAL PROPERTY COMPONENT_SUMMARY_LIST
        BRIEF_DOCS "Component summary list"
        FULL_DOCS "List of component entries (name, status, reason)"
    )
    set_property(GLOBAL PROPERTY COMPONENT_SUMMARY_LIST "")
endfunction()

function(add_plugin_summary name enabled)
    set(reason "")

    # Handle optional third argument
    if(${ARGC} GREATER 2)
        set(reason "${ARGV2}")
    endif()

    if(${enabled})
        set(status "YES")
    else()
        set(status "NO")
    endif()

    # Store triplet as a list entry
    get_property(current GLOBAL PROPERTY PLUGIN_SUMMARY_LIST)
    list(APPEND current "${name}|${status}|${reason}")
    set_property(GLOBAL PROPERTY PLUGIN_SUMMARY_LIST "${current}")
endfunction()

function(add_component_summary name enabled)
    set(reason "")

    # Handle optional third argument
    if(${ARGC} GREATER 2)
        set(reason "${ARGV2}")
    endif()

    if(${enabled})
        set(status "YES")
    else()
        set(status "NO")
    endif()

    # Store triplet as a list entry
    get_property(current GLOBAL PROPERTY COMPONENT_SUMMARY_LIST)
    list(APPEND current "${name}|${status}|${reason}")
    set_property(GLOBAL PROPERTY COMPONENT_SUMMARY_LIST "${current}")
endfunction()

function(str_pad_right result input length)
    string(LENGTH "${input}" input_len)
    math(EXPR padding "${length} - ${input_len}")
    if(padding GREATER 0)
        string(REPEAT " " ${padding} spaces)
        set(${result} "${input}${spaces}" PARENT_SCOPE)
    else()
        set(${result} "${input}" PARENT_SCOPE)
    endif()
endfunction()

function(print_row name status reason)
    set(TABLE_WIDTH 25)
    str_pad_right(name_padded "${name}" ${TABLE_WIDTH})
    str_pad_right(status_padded "${status}" 4)
    if (reason STREQUAL "")
        message(STATUS "  ${name_padded} ${status_padded}")
    else()
        message(STATUS "  ${name_padded} ${status_padded} (${reason})")
    endif()
endfunction()

# Print the complete summary at the end
function(print_summary)
    get_property(plugin_entries GLOBAL PROPERTY PLUGIN_SUMMARY_LIST)
    get_property(component_entries GLOBAL PROPERTY COMPONENT_SUMMARY_LIST)
    message(STATUS "")
    message(STATUS "======================================")
    message(STATUS "  Project Configuration Summary")
    message(STATUS "======================================")

    message(STATUS "Build type:         ${CMAKE_BUILD_TYPE}")
    message(STATUS "Install prefix:     ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "")
    message(STATUS "3rd Party Dependencies:")
    # Qt
    if(${Qt${QT_VERSION_MAJOR}_FOUND})
        print_row("Qt" "YES" "${Qt${QT_VERSION_MAJOR}_VERSION}")
    else()
        print_row("Qt" "NO" "Required")
    endif()
    # OpenCV
    if(${OpenCV_FOUND})
        print_row("OpenCV" "YES" "${OpenCV_VERSION}")
    else()
        print_row("OpenCV" "NO" "Required")
    endif()
    # Onnxruntime
    if(Onnxruntime_FOUND)
        print_row("Onnxruntime" "YES" "${Onnxruntime_VERSION}")
    else()
        print_row("Onnxruntime" "NO" "")
    endif()
    # CUDA
    if(WITH_CUDA)
        print_row("CUDA" "YES" "${CMAKE_CUDA_COMPILER_VERSION}")
    else()
        # If CUDA is not enabled, provide a reason by checking HAS_CUDA and HAS_OPENCV_CUDA
        if(DEFINED HAS_CUDA AND NOT HAS_CUDA)
            set(CUDA_REASON "CUDA compiler not found")
        elseif(DEFINED HAS_OPENCV_CUDA AND NOT HAS_OPENCV_CUDA)
            set(CUDA_REASON "OpenCV with CUDA modules not found")
        else()
            set(CUDA_REASON "Disabled by user")
        endif()

        print_row("CUDA" "NO" "${CUDA_REASON}")
    endif()

    message(STATUS "")
    message(STATUS "Components:")
    foreach(entry IN LISTS component_entries)
        string(REPLACE "|" ";" fields "${entry}")
        list(GET fields 0 name)
        list(GET fields 1 status)
        list(GET fields 2 reason)

        print_row("${name}" "${status}" "${reason}")
    endforeach()

    if(Build_Plugins)
        message(STATUS "")
        message(STATUS "Plugins:")
        foreach(entry IN LISTS plugin_entries)
            string(REPLACE "|" ";" fields "${entry}")
            list(GET fields 0 name)
            list(GET fields 1 status)
            list(GET fields 2 reason)

            print_row("${name}" "${status}" "${reason}")
        endforeach()
    else()
        message(STATUS "")
        message(STATUS "Plugins: Disabled")
    endif()
    message(STATUS "======================================")
    message(STATUS "")
endfunction()
