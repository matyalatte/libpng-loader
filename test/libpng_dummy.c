#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define _EXTERN __declspec(dllexport) extern
#else  // _WIN32
#define _EXTERN __attribute__((visibility("default"))) extern
#endif  // _WIN32

// a dummy library to test LIBPNG_ERROR_VERSION_MISMATCH and LIBPNG_ERROR_FUNCTION_NOT_FOUND
typedef struct png_struct_def png_struct;
_EXTERN char* png_get_libpng_ver(const png_struct *png) {
    return "1.4.0";
}
#ifdef __cplusplus
}
#endif
