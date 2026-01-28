function(add_png_test name source)
    add_executable(${source} ${source}.c)
    target_link_libraries(${source} PRIVATE libpng-loader)
    add_test(
        NAME ${name}
        COMMAND ${source}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${source}>
    )
endfunction()

add_png_test(TestRead test_read)
add_png_test(TestWrite test_write)
add_png_test(TestLoadFail test_load_fail)
if (PNGLOADER_THREAD_SAFE)
    add_png_test(TestThreading test_threading)
endif()
add_custom_command(
    TARGET test_read POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/input.png" $<TARGET_FILE_DIR:test_read>
)

# a dummy library to test LIBPNG_ERROR_VERSION_MISMATCH and LIBPNG_ERROR_FUNCTION_NOT_FOUND
add_library(libpng-dummy SHARED dummy_libs/libpng_dummy.c)
if (PNGLOADER_NO_PREFIX)
    set_target_properties(libpng-dummy PROPERTIES OUTPUT_NAME "png-dummy")
endif()

# a dummy library to test LIBPNG_ERROR_LIBZ_NOT_FOUND
add_library(libpng-dummy-linked SHARED dummy_libs/libpng_dummy_linked.c)
add_library(libz-dummy SHARED dummy_libs/libz_dummy.c)
target_link_libraries(libpng-dummy-linked PRIVATE libz-dummy)
if (PNGLOADER_NO_PREFIX)
    set_target_properties(libpng-dummy-linked PROPERTIES OUTPUT_NAME "png-dummy-linked")
    set_target_properties(libz-dummy PROPERTIES OUTPUT_NAME "z-dummy")
endif()
# remove libz-dummy after linking it to libpng-dummy-linked
add_custom_command(
    TARGET libpng-dummy-linked
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:libz-dummy>
)

# compile a dummy library for a different architecture
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "(x86_64|amd64|AMD64|x64)")
    if (WIN32)
        set(PNGLOADER_CROSS_ARCH "ARM64")
    elseif(APPLE)
        set(PNGLOADER_CROSS_ARCH "arm64")
    else()
        set(PNGLOADER_CROSS_ARCH "aarch64")
    endif()
else()
    if (WIN32)
        set(PNGLOADER_CROSS_ARCH "AMD64")
    else()
        set(PNGLOADER_CROSS_ARCH "x86_64")
    endif()
endif()

set(PNGLOADER_CROSS_TEST ON)
set(PNGLOADER_CROSS_ARGS
    -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DCMAKE_SYSTEM_PROCESSOR=${PNGLOADER_CROSS_ARCH}
)
if (WIN32)
    if(NOT MSVC)
        set(PNGLOADER_CROSS_TEST OFF)
        message(WARNING
            "A test using cross compiled binary was disabled. "
            "Use MSVC to run all tests.")
    endif()
    list(APPEND PNGLOADER_CROSS_ARGS
        -A ${PNGLOADER_CROSS_ARCH}
    )
elseif(APPLE)
    list(APPEND PNGLOADER_CROSS_ARGS
        -DCMAKE_OSX_ARCHITECTURES=${PNGLOADER_CROSS_ARCH}
    )
elseif(LINUX)
    find_program(PNGLOADER_CROSS_COMPILER ${PNGLOADER_CROSS_ARCH}-linux-gnu-gcc)
    if(NOT PNGLOADER_CROSS_COMPILER)
        set(PNGLOADER_CROSS_TEST OFF)
        message(WARNING
            "${PNGLOADER_CROSS_ARCH}-linux-gnu-gcc not found.\n"
            "A test using cross compiled binary was disabled.")
    endif()
    list(APPEND PNGLOADER_CROSS_ARGS
        -DCMAKE_C_COMPILER=${PNGLOADER_CROSS_ARCH}-linux-gnu-gcc
    )
else()
    set(PNGLOADER_CROSS_TEST OFF)
    message(WARNING
        "A test using cross compiled binary was disabled.")
endif()

if(PNGLOADER_CROSS_TEST)
    include(ExternalProject)
    ExternalProject_Add(cross-compiled-test
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/test/cross_compiled_test
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/cross_compiled_test
        INSTALL_COMMAND ""
        CMAKE_ARGS ${PNGLOADER_CROSS_ARGS}
    )

    add_custom_command(
        TARGET cross-compiled-test
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_BINARY_DIR}/cross_compiled_test/lib
            $<TARGET_FILE_DIR:libpng-dummy>
    )
    target_compile_definitions(test_load_fail PRIVATE USE_LIBPNG_DUMMY_CROSS)
endif()
