// generate.py uses this file to generate libpng-loader.h from png.h
#ifndef LIBPNG_LOADER_H
#define LIBPNG_LOADER_H

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// ------ Loader's API ------

/**
 * To enable thread safety, define the `PNGLOADER_THREAD_SAFE` macro.
 * This adds a single, shared mutex that is used by all `libpng_*` functions.
 */
// #define PNGLOADER_THREAD_SAFE

/**
 * Configuration for `libpng_load()`.
 *
 * @enum libpng_load_flags
 */
typedef unsigned int libpng_load_flags;
enum {
    LIBPNG_LOAD_FLAGS_UNSAFE = 0,  //!< Disable all validatiors.
    LIBPNG_LOAD_FLAGS_VERSION_CHECK = 1,  //!< Check the version string of user's libpng.
    LIBPNG_LOAD_FLAGS_FUNCTION_CHECK = 2,  //!< Ensure all function pointers are non-NULL.
    LIBPNG_LOAD_FLAGS_PRINT_ERRORS = 4,  //!< Output error messages to stderr.
};
#define LIBPNG_LOAD_FLAGS_DEFAULT ( \
    LIBPNG_LOAD_FLAGS_VERSION_CHECK | \
    LIBPNG_LOAD_FLAGS_FUNCTION_CHECK)

/**
 * Error code for `libpng_load()`.
 *
 * @enum libpng_load_error
 */
typedef unsigned int libpng_load_error;
enum {
    LIBPNG_SUCCESS = 0,
    LIBPNG_ERROR_LIBPNG_NOT_FOUND,  //!< libpng was not found.
    LIBPNG_ERROR_LIBPNG_INVALID_ELF,  //!< libpng was not built for your platform.
    LIBPNG_ERROR_LIBPNG_FAIL,  //!< failed to load libpng due to other reasons.
    LIBPNG_ERROR_LIBZ_NOT_FOUND,  //!< libz was not found.
    LIBPNG_ERROR_FUNCTION_NOT_FOUND = 32,  //!< a function was not loaded successfully.
    LIBPNG_ERROR_VERSION_MISMATCH,  //!< user's libpng has an unexpected version string.
    LIBPNG_ERROR_LOADED_ALREADY,  //!< libpng was loaded already.
    LIBPNG_ERROR_NULL_REFERENCE,  //!< library path was not specified.
    LIBPNG_ERROR_MAX
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load libpng and its functions.
 * You can use `png_*` functions after loading libpng successfully.
 *
 * @note: If you get `LIBPNG_ERROR_VERSION_MISMATCH`,
 *        you can check the version string with
 *        `libpng_get_user_ver()` and `libpng_get_loader_ver()`.
 *
 * @param flags Configuration flags. Use `LIBPNG_FLAGS_DEFAULT` to enable validations.
 * @returns `LIBPNG_SUCCESS` if all functions are loaded, `LIBPNG_ERROR_*` otherwise.
 */
libpng_load_error libpng_load(libpng_load_flags flags);

/**
 * Load libpng from a file path (e.g. `/usr/lib/libpng16.so`).
 * `dlopen()` and `LoadLibrary()` use the path to load libpng.
 * You can use `png_*` functions after loading libpng successfully.
 *
 * @note: If you get `LIBPNG_ERROR_VERSION_MISMATCH`,
 *        you can check the version string with
 *        `libpng_get_user_ver()` and `libpng_get_loader_ver()`.
 *
 * @param file A file path to libpng. Null pointer is not allowed.
 * @param flags Configuration flags. Use `LIBPNG_FLAGS_DEFAULT` to enable validations.
 * @returns `LIBPNG_SUCCESS` if all functions are loaded, `LIBPNG_ERROR_*` otherwise.
 */
libpng_load_error libpng_load_from_path(const char* file, libpng_load_flags flags);

/**
 * Free libpng and initialize function pointers.
 *
 * @note: `libpng_free()` does not initialize the version string buffer.
 *        `libpng_get_user_ver()` is still available after closing libpng.
 */
void libpng_free(void);

/**
 * Return if libpng is loaded or not.
 *
 * @returns 1 if libpng is loaded, 0 otherwise.
 */
int libpng_is_loaded(void);

/**
 * Return `PNG_LIBPNG_VER_STRING` written in user's libpng.
 * It should be of the form `xx.yy.zz.str`. (e.g. `1.8.0.git`)
 *
 * @note: The `xx.yy` part should be the same as `libpng_get_loader_ver()`
 *        to get `libpng_load()` to work.
 *
 * @note: The returned buffer can be overwritten by `libpng_load()`.
 *
 * @returns A string that represents the version of user's libpng.
 */
const char* libpng_get_user_ver(void);

/**
 * Return `PNG_LIBPNG_VER_STRING` written in `libpng-loader.h`.
 * It should be of the form `xx.yy.zz.str`. (e.g. `1.6.54.libpng-loader`)
 *
 * @note: The `xx.yy` part should be the same as `libpng_get_user_ver()`
 *        to get `libpng_load()` to work.
 *
 * @returns A string that represents the expected version of libpng.
 */
const char* libpng_get_loader_ver(void);

/**
 * Outputs missing functions into stdout
 *
 * @param stream A stream pointer (e.g. stdout) to output messages.
 * @param show_optional Prints all missing APIs when 1.
 *                      Removes optional APIs from the list when 0.
 */
void libpng_print_missing_functions(FILE *stream, int show_optional);

typedef struct png_struct_def png_struct;

// We use a custom implementation of png_init_io,
// because passing FILE* to libpng can cause invalid memory access.
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries?view=msvc-170

// Calls png_set_read_fn with a default callback for reading png files.
void png_init_read_io(png_struct *png_ptr, FILE *fp);

// Calls png_set_write_fn with default callbacks for writing png files.
void png_init_write_io(png_struct *png_ptr, FILE *fp);

#ifdef __cplusplus
}
#endif

// ------ Structs ------

typedef void png_void;
typedef char png_char;
typedef uint8_t png_byte;
typedef int16_t png_int_16;
typedef uint16_t png_uint_16;
typedef int32_t png_int_32;
typedef uint32_t png_uint_32;
typedef size_t png_size_t;
typedef size_t png_alloc_size_t;
typedef png_int_32 png_fixed_point;
typedef double png_double;

typedef struct png_info_def png_info;
typedef struct png_control_def png_control;

// deprecated pointer type aliases
typedef void                  * png_voidp;
typedef const void            * png_const_voidp;
typedef png_byte              * png_bytep;
typedef const png_byte        * png_const_bytep;
typedef png_uint_32           * png_uint_32p;
typedef const png_uint_32     * png_const_uint_32p;
typedef png_int_32            * png_int_32p;
typedef const png_int_32      * png_const_int_32p;
typedef png_uint_16           * png_uint_16p;
typedef const png_uint_16     * png_const_uint_16p;
typedef png_int_16            * png_int_16p;
typedef const png_int_16      * png_const_int_16p;
typedef char                  * png_charp;
typedef const char            * png_const_charp;
typedef png_fixed_point       * png_fixed_point_p;
typedef const png_fixed_point * png_const_fixed_point_p;
typedef size_t                * png_size_tp;
typedef const size_t          * png_const_size_tp;
typedef double       *png_doublep;
typedef const double *png_const_doublep;
typedef png_byte        **png_bytepp;
typedef png_uint_32     **png_uint_32pp;
typedef png_int_32      **png_int_32pp;
typedef png_uint_16     **png_uint_16pp;
typedef png_int_16      **png_int_16pp;
typedef const char      **png_const_charpp;
typedef char            **png_charpp;
typedef png_fixed_point **png_fixed_point_pp;
typedef double          **png_doublepp;
typedef char ***png_charppp;
typedef FILE *png_FILE_p;

typedef png_struct *png_structp;
typedef const png_struct *png_const_structp;
typedef png_struct **png_structpp;
typedef png_struct *png_structrp;
typedef const png_struct *png_const_structrp;

typedef png_info *png_infop;
typedef const png_info *png_const_infop;
typedef png_info **png_infopp;
typedef png_info *png_inforp;
typedef const png_info *png_const_inforp;

typedef png_control *png_controlp;

// ------ Macros ------

#define REMOVE_API(x)

// This macro can apply a macro to all function pointers.
// If you dont want to load some functions, remove "LIBPNG_MAP(png_***) \" from here.
// (or use the REMOVE_API(x) macro.)
// They will be removed from the whole binary of libpng-loader.
// (No need to edit libpng-loader.c!)
// If you dont want to force some functions to be exists, replace LIBPNG_MAP with LIBPNG_OPT.
// They will be ignored when libpng-loader checks fuction pointers.
#define LIBPNG_FUNC_MAPPING \
// ------ Function Pointers ------

// declare all function pointers
#define LIBPNG_MAP(func) extern PFN_##func func;
#define LIBPNG_OPT(func) LIBPNG_MAP(func)
LIBPNG_FUNC_MAPPING
#undef LIBPNG_MAP
#undef LIBPNG_OPT

#endif  // LIBPNG_LOADER_H
