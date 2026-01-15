# libpng-loader

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![donate](https://img.shields.io/static/v1?label=donate&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/matyalatte)  

## About

libpng-loader is a meta loader for libpng16,
similar in concept to [volk for Vulkan](https://github.com/zeux/volk/).
libpng-loader does not require libpng at compile time.
Instead, your app can detect and enable PNG support at runtime based on library availability.

Currently, libpng-loader officially supports 64-bit versions of Windows, Linux, and macOS.
Other platforms may work but may require additional effort to resolve issues.

## Building

Copy `libpng-loader.h` and `libpng-loader.c` into your project and compile them as part of your build.

libpng-loader does not require libpng to be installed at compile time.

## Usage

Call `libpng_load()` once before using any `png_*` functions. After successful loading, most libpng APIs are available through function pointers.

```c
#include "libpng-loader.h"
#include <stdio.h>

int main(void) {
    // Load libpng
    libpng_load_error err = libpng_load(LIBPNG_LOAD_FLAGS_DEFAULT);
    if (err != LIBPNG_SUCCESS) {
        printf("Failed to load libpng\n");
        return 1;
    }

    // You can use png_* functions!
    FILE *fp = fopen("output.png", "wb");
    png_structp png = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    // Note: png_init_io is not available with libpng-loader.
    //       Use png_init_write_io or png_init_read_io instead.
    png_init_write_io(png, fp);

    // Free libpng
    libpng_free();
    return 0;
}
```

## Unsupported Functions

The following functions are intentionally not exposed by libpng-loader.

- `png_init_io`
- `png_image_begin_read_from_stdio`
- `png_image_write_to_stdio`

This is because passing `FILE*` across different C runtimes (especially on Windows) can cause heap corruption.

See:
https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries?view=msvc-170

For `png_init_io`, libpng-loader provides `png_init_write_io` and `png_init_read_io` instead.

## Optional Functions

The following functions may be `NULL` even after successfully loading libpng. This typically depends on the libpng version available at runtime.

You must check for `NULL` before calling any of these functions:

- `png_get_cICP`
- `png_set_cICP`
- `png_get_cLLI`
- `png_get_cLLI_fixed`
- `png_set_cLLI`
- `png_set_cLLI_fixed`
- `png_get_eXIf`
- `png_set_eXIf`
- `png_get_eXIf_1`
- `png_set_eXIf_1`
- `png_get_mDCV`
- `png_get_mDCV_fixed`
- `png_set_mDCV`
- `png_set_mDCV_fixed`
- `png_set_strip_error_numbers`

## Remove Functions

You can exclude specific libpng APIs from the final binary by modifying the `LIBPNG_FUNC_MAPPING` macro in `libpng-loader.h`.

Remove the corresponding `LIBPNG_MAP(png_*)` entry or replace it with `REMOVE_API(png_*)`. This removes the function pointer and all related code at compile time.

```c
// png_set_sig_bytes will be removed!
#define LIBPNG_FUNC_MAPPING \
    LIBPNG_MAP(png_access_version_number) \
    REMOVE_API(png_set_sig_bytes) \
    LIBPNG_MAP(png_sig_cmp) \
    LIBPNG_MAP(png_create_read_struct) \
    ...
```
