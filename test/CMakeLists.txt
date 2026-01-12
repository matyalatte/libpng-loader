function(add_png_test name source)
    add_executable(${source} ${source}.c)
    target_link_libraries(${source} PRIVATE libpng-loader)
    add_test(
        NAME ${name}
        COMMAND ${source}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${source}>
    )
endfunction()

add_png_test(TestWrite test_write)
add_png_test(TestLoadFail test_load_fail)

# a dummy library to test LIBPNG_ERROR_VERSION_MISMATCH and LIBPNG_ERROR_FUNCTION_NOT_FOUND
add_library(libpng-dummy SHARED dummy_libs/libpng_dummy.c)
if (NOT WIN32)
    set_target_properties(libpng-dummy PROPERTIES OUTPUT_NAME "png-dummy")
endif()

# a dummy library to test LIBPNG_ERROR_LIBZ_NOT_FOUND
add_library(libpng-dummy-linked SHARED dummy_libs/libpng_dummy_linked.c)
add_library(libz-dummy SHARED dummy_libs/libz_dummy.c)
target_link_libraries(libpng-dummy-linked PRIVATE libz-dummy)
if (NOT WIN32)
    set_target_properties(libpng-dummy-linked PROPERTIES OUTPUT_NAME "png-dummy-linked")
    set_target_properties(libz-dummy PROPERTIES OUTPUT_NAME "z-dummy")
endif()
# remove libz-dummy after linking it to libpng-dummy-linked
add_custom_command(
    TARGET libpng-dummy-linked
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:libz-dummy>
)
