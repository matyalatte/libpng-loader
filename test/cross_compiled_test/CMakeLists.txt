cmake_minimum_required(VERSION 3.10)

project(cross-compiled-test LANGUAGES C)

add_library(libpng-dummy-cross SHARED ../dummy_libs/libpng_dummy.c)
set_target_properties(libpng-dummy-cross PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
)
if (NOT WIN32)
    set_target_properties(libpng-dummy-cross PROPERTIES OUTPUT_NAME "png-dummy-cross")
endif()
