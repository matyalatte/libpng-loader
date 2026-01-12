#include "libpng-loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

static void png_write_data(png_struct *png_ptr, png_byte *data, png_size_t length) {
    FILE* io = (FILE*)png_get_io_ptr(png_ptr);
    fwrite(data, sizeof(png_byte), length, io);
}

static void png_flush_data(png_struct *png_ptr) {
}

static int write_png(const char* filename) {
    int width = 300, height = 250;
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "failed to open %s\n", filename);
        return 1;
    }

    // Initialize write structure
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "failed to create png_struct\n");
        fclose(fp);
        return 1;
    }
    png_infop info = png_create_info_struct(png);

    // TODO: add this macro to libpng-loader.h
    #  define png_jmpbuf(png_ptr) \
        (*png_set_longjmp_fn((png_ptr), longjmp, (sizeof (jmp_buf))))

    if (setjmp(png_jmpbuf(png))) {
        // libpng jumped here due to an error
        fprintf(stderr, "failed to output png image\n");
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return 1;
    }

    png_set_write_fn(png, (png_void*)fp, png_write_data, png_flush_data);

    // Set image attributes (8-bit color depth, RGBA)
    png_set_IHDR(
        png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // Allocate and fill row data
    png_byte* row = (png_byte*) malloc(4 * width * sizeof(png_byte));
    if (!row) {
        fprintf(stderr, "failed to allocate png_byte buffer.\n");
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return 1;
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            row[x*4 + 0] = (png_byte)((double)x / (double)width * 255);
            row[x*4 + 1] = 255;
            row[x*4 + 2] = (png_byte)((double)y / (double)height * 255);
            row[x*4 + 3] = 255;
        }
        png_write_row(png, row);
    }

    // Cleanup
    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return 0;
}

int main(void) {
    libpng_load_error err = libpng_load(LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_PRINT_ERRORS);
    if (err != LIBPNG_SUCCESS) {
        fprintf(stderr, "libpng_load: error: %d\n", err);
        return 1;
    }
    if (libpng_is_loaded() != 1) {
        fprintf(stderr, "libpng_is_loaded: not 1\n");
        return 1;
    }
    err = write_png("output.png");
    libpng_free();
    if (err == 0)
        printf("Test passed!\n");
    return err;
}
