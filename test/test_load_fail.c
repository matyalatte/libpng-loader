#include "libpng-loader.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define LIB_EXT ".dll"
#elif defined(__APPLE__)
#define LIB_EXT ".dylib"
#else
#define LIB_EXT ".so"
#endif

int main(void) {
    libpng_load_error err;

    // Test libpng_get_*_ver()
    const char* ver = libpng_get_loader_ver();
    if (strcmp(ver, PNG_LIBPNG_VER_STRING) != 0) {
        fprintf(stderr, "libpng_get_user_ver: unexpected value: %s", ver);
        return 1;
    }
    ver = libpng_get_user_ver();
    if (strcmp(ver, "0.0.0") != 0) {
        fprintf(stderr, "libpng_get_user_ver: unexpected value: %s", ver);
        return 1;
    }

    // Test with null pointer
    err = libpng_load_from_path(NULL, LIBPNG_LOAD_FLAGS_DEFAULT);
    if (err != LIBPNG_ERROR_NULL_REFERENCE) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_NULL_REFERENCE: %d\n", err);
        return 1;
    }

    // Test with fake path
    err = libpng_load_from_path("libpng-not-found" LIB_EXT, LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_ERROR_LIBPNG_NOT_FOUND) {
        fprintf(stderr, "libpng_load: unexpected error code: %d\n", err);
        return 1;
    }

    // The library exists but a linked library is missing
    err = libpng_load_from_path("./libpng-dummy-linked" LIB_EXT, LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_ERROR_LIBZ_NOT_FOUND) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_LIBZ_NOT_FOUND: %d\n", err);
        return 1;
    }

#ifdef USE_LIBPNG_DUMMY_CROSS
    // The library is built for a wrong architecture
    err = libpng_load_from_path("./libpng-dummy-cross" LIB_EXT, LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_ERROR_LIBPNG_INVALID_ELF) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_LIBPNG_INVALID_ELF: %d\n", err);
        return 1;
    }
#endif

    // Test with invalid version
    err = libpng_load_from_path("./libpng-dummy" LIB_EXT, LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_ERROR_VERSION_MISMATCH) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_VERSION_MISMATCH: %d\n", err);
        return 1;
    }
    // Check if we can get the mismatched version string.
    ver = libpng_get_user_ver();
    if (strcmp(ver, "1.4.0") != 0) {
        fprintf(stderr, "libpng_get_user_ver: unexpected value: %s", ver);
        return 1;
    }

    // Test with missing functions
    err = libpng_load_from_path("./libpng-dummy" LIB_EXT, LIBPNG_LOAD_FLAGS_FUNCTION_CHECK);
    if (err != LIBPNG_ERROR_FUNCTION_NOT_FOUND) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_FUNCTION_NOT_FOUND: %d\n", err);
        return 1;
    }

    // Test without validations
    err = libpng_load_from_path("./libpng-dummy" LIB_EXT, LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_SUCCESS) {
        fprintf(stderr, "libpng_load: not LIBPNG_SUCCESS: %d\n", err);
        return 1;
    }

    // Try to load libpng twice
    err = libpng_load_from_path("./libpng-dummy" LIB_EXT, LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_ERROR_LOADED_ALREADY) {
        fprintf(stderr, "libpng_load: not LIBPNG_ERROR_LOADED_ALREADY: %d\n", err);
        return 1;
    }

    libpng_free();
    printf("Test passed!\n");
    return 0;
}
