cmake_minimum_required(VERSION 3.10)

project(libpng-loader LANGUAGES C)

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

enable_testing()
add_executable(load_test  test/load_test.c)
target_link_libraries(load_test PRIVATE libpng-loader)
if (NOT WIN32)
    target_link_libraries(load_test PRIVATE ${CMAKE_DL_LIBS})
endif()
add_test(NAME LoadTest COMMAND load_test)
