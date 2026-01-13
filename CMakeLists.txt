cmake_minimum_required(VERSION 3.10)

project(libpng-loader LANGUAGES C)

option(BUILD_TEST "Build tests" ON)

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
    set_target_properties(libpng-loader PROPERTIES OUTPUT_NAME "png-loader")
endif()

if (BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
