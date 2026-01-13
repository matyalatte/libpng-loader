#include "libpng-loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_png(const char* filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "failed to open %s\n", filename);
        return 1;
    }

    png_byte signature[8];
    size_t sig_size = fread(signature, 1, 8, fp);
    if(png_sig_cmp(signature, 0, 8)) {
        fprintf(stderr, "not png file\n");
        fclose(fp);
        return 1;
    }
    // Initialize read structure
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "failed to create png_struct\n");
        fclose(fp);
        return 1;
    }
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        // libpng jumped here due to an error
        fprintf(stderr, "failed to read png image\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    png_init_read_io(png, fp);
    png_set_sig_bytes(png, (int)sig_size);
    png_read_png(png, info, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16, NULL);

    unsigned int width, height;
    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    png_byte** datap = png_get_rows(png, info);
    png_byte type = png_get_color_type(png, info);

    if(type != PNG_COLOR_TYPE_RGB && type != PNG_COLOR_TYPE_RGB_ALPHA){
        printf("color type is not RGB or RGBA\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return -1;
    }

    unsigned int channel = 4;
    if (type == PNG_COLOR_TYPE_RGB) {
        channel = 3;
    }

    // Allocate and copy row data
    png_byte* row = (png_byte*) malloc(sizeof(png_byte) * width * channel);
    if (!row) {
        fprintf(stderr, "failed to allocate png_byte buffer.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    // check pixels
    for(int y = 0; y < (int)height; y++){
        memcpy(row, datap[y], width * channel);
        for (int x = 0; x < (int)width; x++) {
            if (row[x*4 + 0] != (png_byte)((double)x / (double)width * 255) ||
                row[x*4 + 1] != 255 ||
                row[x*4 + 2] != (png_byte)((double)y / (double)height * 255) ||
                row[x*4 + 3] != 255) {
                fprintf(stderr, "unexpected pixel data detected.\n");
                free(row);
                png_destroy_read_struct(&png, &info, NULL);
                fclose(fp);
                return 1;
            }
        }
    }

    // Cleanup
    free(row);
    png_destroy_read_struct(&png, &info, NULL);
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
    err = read_png("input.png");
    libpng_free();
    if (err == 0)
        printf("Test passed!\n");
    return err;
}
