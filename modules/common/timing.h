#ifndef TIMING_H
#define TIMING_H

/*
 * Wall-clock timing helper used across the application.
 * Algorithm handlers measure running time with timer_start/timer_stop_ms
 * so every endpoint can report running_time_ms.
 */

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

typedef struct {
#ifdef _WIN32
    LARGE_INTEGER start;
#else
    struct timespec start;
#endif
} Timer;

static inline void timer_start(Timer *t)
{
#ifdef _WIN32
    QueryPerformanceCounter(&t->start);
#else
    clock_gettime(CLOCK_MONOTONIC, &t->start);
#endif
}

static inline double timer_stop_ms(const Timer *t)
{
#ifdef _WIN32
    LARGE_INTEGER end, freq;
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);
    return (double)(end.QuadPart - t->start.QuadPart) * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (double)(end.tv_sec - t->start.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - t->start.tv_nsec) / 1000000.0;
#endif
}

#endif
