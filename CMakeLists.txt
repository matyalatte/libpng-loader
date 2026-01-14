cmake_minimum_required(VERSION 3.10)

project(libpng-loader LANGUAGES C)

option(PNGLOADER_TESTS "Build tests" ON)

# Warnings for unsupported environments
if(NOT (WIN32 OR APPLE OR LINUX))
    message(WARNING
        "libpng-loader is not tested on your platform.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING
        "libpng-loader is tested on 64-bit systems. It might not work on your machine.")
endif()

# Main library
add_library(libpng-loader STATIC
    libpng-loader.h
    libpng-loader.c
)
target_include_directories(libpng-loader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)
if (NOT WIN32)
    target_link_libraries(libpng-loader PUBLIC ${CMAKE_DL_LIBS})
endif()

# Remove "lib" from the library name because your toolchain adds it.
if (UNIX
    OR (WIN32 AND NOT CMAKE_SHARED_LIBRARY_PREFIX STREQUAL "")
    OR (WIN32 AND NOT CMAKE_STATIC_LIBRARY_PREFIX STREQUAL ""))
    set(PNGLOADER_NO_PREFIX ON)
    set_target_properties(libpng-loader PROPERTIES OUTPUT_NAME "png-loader")
endif()

# Build tests
if (PNGLOADER_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
