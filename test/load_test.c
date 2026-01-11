#include "libpng-loader.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    libpng_load_error err = libpng_load(LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_SUCCESS) {
        fprintf(stderr, "libpng_load: error: %d", err);
        return 1;
    }
    if (libpng_is_loaded() != 1) {
        fprintf(stderr, "libpng_is_loaded: not 1");
        return 1;
    }
    libpng_free();
    printf("Test passed!\n");
    return 0;
}
