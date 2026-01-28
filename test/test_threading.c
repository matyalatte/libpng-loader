#ifdef __linux__
#define _GNU_SOURCE
#endif
#include "libpng-loader.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// Test if we can call libpng_load() from multiple threads

#define NUM_THREADS 8

typedef struct {
    int thread_id;
    int cpu_id;
    int result;
} thread_data_t;

#ifdef _WIN32
typedef HANDLE thread_t;
SYNCHRONIZATION_BARRIER barrier;
DWORD WINAPI thread_func(LPVOID arg) {
    thread_data_t* data = (thread_data_t*)arg;

    HANDLE hthread = GetCurrentThread();
    SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);
    SetThreadAffinityMask(hthread, 1ULL << data->cpu_id);
    Sleep(10);
    EnterSynchronizationBarrier(&barrier, 0);

    libpng_load_error err;
    err = libpng_load(LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_FAIL_IF_LOADED);
    data->result = (err == LIBPNG_SUCCESS) ? 1 : 0;

    return 0;
}
#else
typedef pthread_t thread_t;
#ifdef __linux__
pthread_barrier_t barrier;
#endif
void* thread_func(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(data->cpu_id, &cpuset);
    pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &cpuset);
#endif
    usleep(10 * 1000);
#ifdef __linux__
    pthread_barrier_wait(&barrier);
#endif

    libpng_load_error err;
    err = libpng_load(LIBPNG_LOAD_FLAGS_DEFAULT | LIBPNG_LOAD_FLAGS_FAIL_IF_LOADED);
    data->result = (err == LIBPNG_SUCCESS) ? 1 : 0;

    pthread_exit(NULL);
}
#endif

int main(void) {

    thread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    int sum = 0;
    int cpu_count = 1;

    // initialize barrier and get cpu count
#ifdef _WIN32
    InitializeSynchronizationBarrier(&barrier, NUM_THREADS, -1);
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    cpu_count = (int)sysinfo.dwNumberOfProcessors;
#else
#ifdef __linux__
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    cpu_count = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
#endif

    // create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].result = 0;
        thread_data[i].cpu_id = i % cpu_count;

        int err;
    #ifdef _WIN32
        threads[i] = CreateThread(
            NULL,
            0,
            thread_func,
            &thread_data[i],
            0,
            NULL
        );
        err = threads[i] == NULL;
    #else
        int ret = pthread_create(
            &threads[i],
            NULL,
            thread_func,
            &thread_data[i]
        );
        err = ret != 0;
    #endif
        if (err) {
            fprintf(stderr, "Failed to create a thread.\n");
            return 1;
        }
    }

    // wait for all threads to finish, and collect results
#ifdef _WIN32
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    for (int i = 0; i < NUM_THREADS; i++) {
        sum += thread_data[i].result;
        CloseHandle(threads[i]);
    }
    DeleteSynchronizationBarrier(&barrier);
#else
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        sum += thread_data[i].result;
    }
#ifdef __linux__
    pthread_barrier_destroy(&barrier);
#endif
#endif

    if (sum != 1) {
        fprintf(stderr, "libpng was loaded multiple times: %d\n", sum);
        return 1;
    }
    return 0;
}
