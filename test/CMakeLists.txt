function(add_png_test name source)
    add_executable(${source} ${source}.c)
    target_link_libraries(${source} PRIVATE libpng-loader)
    add_test(
        NAME ${name}
        COMMAND ${source}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endfunction()

add_png_test(TestWrite test_write)
add_png_test(TestLoadFail test_load_fail)

add_library(libpng-dummy SHARED
    libpng_dummy.c
)
if (NOT WIN32)
    set_target_properties(libpng-dummy PROPERTIES OUTPUT_NAME "png-dummy")
endif()
