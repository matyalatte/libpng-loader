#include "libpng-loader.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <string.h>
#endif

static void* libpng_ptr = NULL;
static char libpng_ver_str[16] = {0};
#define LIBPNG_VER_STR_SIZE (sizeof(libpng_ver_str) / sizeof(char))

// copies the version string to libpng_ver_str
static void copy_to_libpng_ver_str(const char* ver_str) {
    int i = 0;
    while(ver_str[i] != 0 && (i < LIBPNG_VER_STR_SIZE - 1)) {
        libpng_ver_str[i] = ver_str[i];
        i++;
    }
    libpng_ver_str[i] = 0;
}

// returns if the version string has the expected minor version or not.
static int is_expected_libpng_version(const char* ver_str) {
    int i = -1;
    int found_dots = 0;
    do {
        i++;
        if (ver_str[i] != PNG_LIBPNG_VER_STRING[i])
            return 0;
        if (ver_str[i] == '.')
            found_dots++;
    } while (found_dots < 2 && ver_str[i] != 0 && PNG_LIBPNG_VER_STRING[i] != 0);
    return 1;
}

static int functions_are_loaded(void) {
    // check if all function pointers are not null.
    #define LIBPNG_MAP(func_name) (func_name != NULL) &&
    #define LIBPNG_OPT(func_name)
    return (LIBPNG_FUNC_MAPPING 1);
    #undef LIBPNG_MAP
    #undef LIBPNG_OPT
}

#ifdef _WIN32
int dll_exists(const char* name) {
    // This can load dll even when one of its dependencies is missing
    HMODULE handle = LoadLibraryExA(name, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (handle) {
        FreeLibrary(handle);
        return 1;
    }
    return 0;
}

static libpng_load_error open_library(const char *name, void** lib_ptr, int print_errors) {
        *lib_ptr = (void*)LoadLibraryA(name);
    if (*lib_ptr)
        return LIBPNG_SUCCESS;
    // Find the cause of the failure
    DWORD winerr = GetLastError();
    if (print_errors) {
        LPSTR message = NULL;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            winerr,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            (LPSTR)&message,
            0,
            NULL);

        if (message) {
            fprintf(stderr, "LIBPNG_ERROR: %s: %s", name, message);
            LocalFree(message);
        }
    }
    if (winerr == ERROR_MOD_NOT_FOUND) {
        if (dll_exists(name)) {
            // If libpng exists, zlib.dll should be missing.
            if (print_errors)
                fprintf(stderr, "LIBPNG_ERROR: %s exists but zlib.dll is missing.\n", name);
            return LIBPNG_ERROR_LIBZ_NOT_FOUND;
        }
        return LIBPNG_ERROR_LIBPNG_NOT_FOUND;
    }
    if (winerr == ERROR_BAD_EXE_FORMAT)
        return LIBPNG_ERROR_LIBPNG_INVALID_ELF;
    return LIBPNG_ERROR_LIBPNG_FAIL;
}
#define DL_CLOSE(ptr) FreeLibrary(ptr)
#define DL_SYM(lib_ptr, name) (void(*)(void))GetProcAddress(lib_ptr, name)
#else
static libpng_load_error open_library(const char *name, void** lib_ptr, int print_errors) {
    *lib_ptr = dlopen(name, RTLD_NOW | RTLD_LOCAL);
    if (*lib_ptr)
        return LIBPNG_SUCCESS;

    // Parse dlerror() to find the cause of the failure.
    const char* err_str = dlerror();
    if (err_str) {
        if (print_errors)
            fprintf(stderr, "LIBPNG_ERROR: %s\n", err_str);
        if (strstr(err_str, "o such file") != 0) {
            // No such file or directory
            if (strstr(err_str, "libz") != 0)
                return LIBPNG_ERROR_LIBZ_NOT_FOUND;
            return LIBPNG_ERROR_LIBPNG_NOT_FOUND;
        } else if (strstr(err_str, "nvalid ELF")){
            // Invalid ELF Header
            return LIBPNG_ERROR_LIBPNG_INVALID_ELF;
        }
    }
    return LIBPNG_ERROR_LIBPNG_FAIL;
}
#define DL_CLOSE(ptr) dlclose(ptr)
#define DL_SYM(lib_ptr, name) dlsym(lib_ptr, name)
#endif

static void load_functions(void) {
    // apply dlsym to all libpng functions
    #define LIBPNG_MAP(func) func = (PFN_##func)DL_SYM(libpng_ptr, #func);
    #define LIBPNG_OPT(func) LIBPNG_MAP(func)
    LIBPNG_FUNC_MAPPING
    #undef LIBPNG_MAP
    #undef LIBPNG_OPT
}

static libpng_load_error libpng_load_base(const char* file, libpng_load_flags flags) {
    int print_errors = flags & LIBPNG_LOAD_FLAGS_PRINT_ERRORS;
    if (libpng_is_loaded()) {
        if (print_errors)
            fprintf(stderr, "LIBPNG_ERROR: libpng is loaded laready.\n");
        return LIBPNG_ERROR_LOADED_ALREADY;
    }

    #ifdef _WIN32
    #define LIB_EXT ".dll"
    #elif defined(__APPLE__)
    #define LIB_EXT ".dylib"
    #else
    #define LIB_EXT ".so"
    #endif

    libpng_load_error err;
    if (file) {
        err = open_library(file, &libpng_ptr, print_errors);
    } else {
        // Find libpng16.so
        err = open_library("libpng16" LIB_EXT, &libpng_ptr, print_errors);
        #ifdef __APPLE__
        // macOS does not search these folders
        if (!libpng_ptr && err == LIBPNG_ERROR_LIBPNG_NOT_FOUND)
            err = open_library("/usr/local/lib/libpng16" LIB_EXT, &libpng_ptr, print_errors);
        if (!libpng_ptr && err == LIBPNG_ERROR_LIBPNG_NOT_FOUND)
            err = open_library("/opt/homebrew/lib/libpng16" LIB_EXT, &libpng_ptr, print_errors);
        #else
        // Find libpng.so
        if (!libpng_ptr && err == LIBPNG_ERROR_LIBPNG_NOT_FOUND)
            err = open_library("libpng" LIB_EXT, &libpng_ptr, print_errors);
        #endif
    }

    if (!libpng_ptr)
        return err;

    load_functions();
    if (!png_get_libpng_ver) {
        libpng_free();
        if (print_errors)
            fprintf(stderr, "LIBPNG_ERROR: png_get_libpng_ver is missing.\n");
        return LIBPNG_ERROR_FUNCTION_NOT_FOUND;
    }
    const char *ver_str = png_get_libpng_ver(NULL);
    // store the libpng version to get it after returning LIBPNG_ERROR_VERSION_MISMATCH
    copy_to_libpng_ver_str(ver_str);

    // check the compatibility
    if ((flags & LIBPNG_LOAD_FLAGS_VERSION_CHECK) &&
            !is_expected_libpng_version(ver_str)) {
        // We should use the same minor version of libpng as PNG_LIBPNG_VER_STRING
        if (print_errors) {
            fprintf(stderr,
                "LIBPNG_ERROR: libpng %s is not supported. It should be %d.%d.x.\n",
                ver_str,
                PNG_LIBPNG_VER_MAJOR,
                PNG_LIBPNG_VER_MINOR);
        }
        libpng_free();
        return LIBPNG_ERROR_VERSION_MISMATCH;
    }

    if ((flags & LIBPNG_LOAD_FLAGS_FUNCTION_CHECK) &&
            !functions_are_loaded()) {
        if (print_errors) {
            fprintf(stderr, "LIBPNG_ERROR: ");
            libpng_print_missing_functions(stderr, 0);
        }
        libpng_free();
        return LIBPNG_ERROR_FUNCTION_NOT_FOUND;
    }

    return LIBPNG_SUCCESS;
}

libpng_load_error libpng_load(libpng_load_flags flags) {
    return libpng_load_base(NULL, flags);
}

libpng_load_error libpng_load_from_path(const char* file, libpng_load_flags flags) {
    if (!file)
        return LIBPNG_ERROR_NULL_REFERENCE;
    return libpng_load_base(file, flags);
}

void libpng_free(void) {
    // set NULL to all function pointers.
    #define LIBPNG_MAP(func_ptr) func_ptr = NULL;
    #define LIBPNG_OPT(func_ptr) LIBPNG_MAP(func_ptr);
    LIBPNG_FUNC_MAPPING
    #undef LIBPNG_MAP
    #undef LIBPNG_OPT

    if (libpng_ptr)
        DL_CLOSE(libpng_ptr);
    libpng_ptr = NULL;
}

int libpng_is_loaded(void) {
    return (libpng_ptr == NULL) ? 0 : 1;
}

const char* libpng_get_user_ver(void) {
    if (png_get_libpng_ver)
        return png_get_libpng_ver(NULL);
    if (libpng_ver_str[0] != '\0')
        return libpng_ver_str;
    return "0.0.0";
}

const char* libpng_get_header_ver(void) {
    return PNG_LIBPNG_VER_STRING;
}

void libpng_print_missing_functions(FILE *stream, int show_optional) {
    fprintf(stream, "missing functions:\n");
    int missing = 0;
    #define LIBPNG_MAP(func) \
        if (func == NULL) \
            fprintf(stream, "  %s\n", #func); missing = 1;
    #define LIBPNG_OPT(func) \
        if (show_optional && func == NULL) \
            fprintf(stream, "  %s (optional)\n", #func); missing = 1;
    LIBPNG_FUNC_MAPPING
    #undef LIBPNG_MAP
    #undef LIBPNG_OPT

    if (missing == 0)
        fprintf(stream, "  (none)");
}

// declare all function pointers
#define LIBPNG_MAP(func) PFN_##func func = NULL;
#define LIBPNG_OPT(func_ptr) LIBPNG_MAP(func_ptr);
LIBPNG_FUNC_MAPPING
#undef LIBPNG_MAP
#undef LIBPNG_OPT
