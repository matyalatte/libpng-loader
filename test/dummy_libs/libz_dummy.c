#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define _EXTERN __declspec(dllexport) extern
#else  // _WIN32
#define _EXTERN __attribute__((visibility("default"))) extern
#endif  // _WIN32

// a dummy library to test LIBPNG_ERROR_LIBZ_NOT_FOUND
_EXTERN void zlib_dummy() {
}
#ifdef __cplusplus
}
#endif
