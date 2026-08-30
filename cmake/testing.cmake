function(target_tests TARGET)

    if (NOT BUILD_TESTING)
        return()
    endif()

    set(TEST_SOURCES ${ARGN})

    foreach(_test_file IN LISTS TEST_SOURCES)
        cmake_path(GET _test_file STEM LAST_ONLY _test_name)
        add_executable("${_test_name}" "${_test_file}")
        target_link_libraries("${_test_name}" PRIVATE ${TARGET})
        add_test(NAME "${_test_name}" COMMAND "${_test_name}")
    endforeach()

endfunction()