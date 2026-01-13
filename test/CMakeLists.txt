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

# compile a dummy library for a different architecture
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "(x86_64|amd64|AMD64|x64)")
    if (WIN32)
        set(CROSS_SYSTEM_PROCESSOR "ARM64")
    elseif(APPLE)
        set(CROSS_SYSTEM_PROCESSOR "arm64")
    else()
        set(CROSS_SYSTEM_PROCESSOR "aarch64")
    endif()
else()
    if (WIN32)
        set(CROSS_SYSTEM_PROCESSOR "AMD64")
    else()
        set(CROSS_SYSTEM_PROCESSOR "x86_64")
    endif()
endif()

set(BUILD_CROSS_COMPILE_TEST ON)
set(CROSS_CMAKE_ARGS
    -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DCMAKE_SYSTEM_PROCESSOR=${CROSS_SYSTEM_PROCESSOR}
)
if (WIN32)
    if(NOT MSVC)
        set(BUILD_CROSS_COMPILE_TEST OFF)
        message(WARNING
            "A test using cross compiled binary was disabled. "
            "Use MSVC to run all tests.")
    endif()
    list(APPEND CROSS_CMAKE_ARGS
        -A ${CROSS_SYSTEM_PROCESSOR}
    )
elseif(APPLE)
    list(APPEND CROSS_CMAKE_ARGS
        -DCMAKE_OSX_ARCHITECTURES=${CROSS_SYSTEM_PROCESSOR}
    )
elseif(LINUX)
    find_program(CROSS_COMPILER ${CROSS_SYSTEM_PROCESSOR}-linux-gnu-gcc)
    if(NOT CROSS_COMPILER)
        set(BUILD_CROSS_COMPILE_TEST OFF)
        message(WARNING
            "${CROSS_SYSTEM_PROCESSOR}-linux-gnu-gcc not found.\n"
            "A test using cross compiled binary was disabled.")
    endif()
    list(APPEND CROSS_CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CROSS_SYSTEM_PROCESSOR}-linux-gnu-gcc
    )
else()
    set(BUILD_CROSS_COMPILE_TEST OFF)
    message(WARNING
        "A test using cross compiled binary was disabled.")
endif()

if(BUILD_CROSS_COMPILE_TEST)
    include(ExternalProject)
    ExternalProject_Add(cross-compiled-test
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/test/cross_compiled_test
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/cross_compiled_test
        INSTALL_COMMAND ""
        BUILD_ALWAYS TRUE
        CMAKE_ARGS ${CROSS_CMAKE_ARGS}
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
