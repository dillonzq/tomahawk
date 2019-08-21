/**
 * A C library to get high precision(origin of the name Tomahawk) timers through
 * reading time stamp counter for platform with modern CPUs
 * or calling clock_gettime for POSIX compliant systems with older CPUs.
 *
 * Author: Dillon <dillonzq@outlook.com>
 * Copyright: This code is made public using the MIT License.
 */

#if !defined(__amd64) && !defined(__i386)
#error "Architecture not supported"
#endif

#ifndef TOMAHAWK_H
#define TOMAHAWK_H

#define _GNU_SOURCE

#include <math.h>
#include <stdint.h>
#include <time.h>

#if __has_include(<cpuid.h>)
#include <cpuid.h>
#else
#define __CPUID(level, a, b, c, d)              \
__asm__ ("cpuid\n\t"                            \
       : "=a" (a), "=b" (b), "=c" (c), "=d" (d) \
       : "0" (level))

static inline int
__get_cpuid (unsigned int __leaf,
             unsigned int *__eax, unsigned int *__ebx,
             unsigned int *__ecx, unsigned int *__edx)
{
    unsigned int __ext = __leaf & 0x80000000;
    __CPUID (__ext, *__eax, *__ebx, *__ecx, *__edx);

    if (*__eax == 0 || *__eax < __leaf)
        return 0;

    __CPUID (__leaf, *__eax, *__ebx, *__ecx, *__edx);
    return 1;
}
#endif

typedef enum th_status_t
{
    TOMAHAWK_NOT_USE = 0,
    TOMAHAWK_USE_RDTSCP,
    TOMAHAWK_USE_RDTSCP_MEAS,
    TOMAHAWK_USE_RDTSC,
    TOMAHAWK_USE_RDTSC_MEAS,
    TOMAHAWK_USE_CLOCK_GETTIME,
} th_status_t;

typedef enum th_error_t
{
    TOMAHAWK_NO_ERROR = 0,
    TOMAHAWK_NOT_USE_ERROR,
    TOMAHAWK_TIMER_NO_BEGIN_ERROR,
    TOMAHAWK_TIMER_OVERFLOW_ERROR,
} th_error_t;

typedef struct th_timer
{
    uint64_t begin;
    uint64_t end;
    struct timespec __clock_begin;
    struct timespec __clock_end;
} th_timer;

static th_status_t __th_status = TOMAHAWK_NOT_USE;
static uint64_t __th_cpu_hz;

#define __NSEC_PER_SEC 1000000000ULL
#define __USEC_PER_SEC 1000000
#define __MSEC_PER_SEC 1000

#define __TH_CALIBRATE_TIMES 1000000
static uint64_t __th_overhead = 0;
static double __th_relative_error = 100;


/*
 *    These macros are used for benchmark by RDTSC on Intel IA-64 and IA-32
 *
 * The first CPUID call implements a barrier toavoid out-of-order execution of
 * the instructions above and below the RDTSC instruction.
 * The RDTSC then reads the timestamp register and the value is stored in memory.
 * The RDTSCP instruction reads the timestamp register for the second time and
 * guarantees that the execution of all the code we wanted to measure is completed.
 * Finally a CPUID call guarantees that a barrier is implemented again so that
 * it is impossible that any instruction coming afterwards is executed before CPUID.
 *
 * based on: How to Benchmark Code Execution Times on Intel ® IA-32 and IA-64 Instruction Set Architectures
 * https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
 */
#ifdef __amd64
#define __TH_RDTSC_BEGIN(tsc)                             \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("cpuid\n\t"                      \
                         "rdtsc\n\t"                      \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%rax", "%rbx", "%rcx", "%rdx"); \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)
#define __TH_RDTSCP_END(tsc)                              \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("rdtscp\n\t"                     \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                         "cpuid\n\t"                      \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%rax", "%rbx", "%rcx", "%rdx"); \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)
#define __TH_RDTSC_END(tsc)                               \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("mov %%cr0, %%rax\n\t"           \
                         "mov %%rax, %%cr0\n\t"           \
                         "rdtsc\n\t"                      \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%rax", "%rdx");                 \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)

#else
#define __TH_RDTSC_BEGIN(tsc)                             \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("cpuid\n\t"                      \
                         "rdtsc\n\t"                      \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%eax", "%ebx", "%ecx", "%edx"); \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)
#define __TH_RDTSCP_END(tsc)                              \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("rdtscp\n\t"                     \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                         "cpuid\n\t"                      \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%eax", "%ebx", "%ecx", "%edx"); \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)
#define __TH_RDTSC_END(tsc)                               \
do {                                                      \
    unsigned int tsc_high, tsc_low;                       \
    __asm__ __volatile__("mov %%cr0, %%rax\n\t"           \
                         "mov %%rax, %%cr0\n\t"           \
                         "rdtsc\n\t"                      \
                         "mov %%edx, %0\n\t"              \
                         "mov %%eax, %1\n\t"              \
                       : "=r" (tsc_high), "=r" (tsc_low): \
                       : "%eax", "%edx");                 \
	(tsc) = ((uint64_t) tsc_high << 32U) | tsc_low;       \
} while (0)
#endif


/*
 * get_tsc_khz_cpuid
 *    This function returns the native frequency of the TSC in kHz via CPUID
 *
 * based on: Intel® 64 and IA-32 Architectures Software Developer’s Manual Vol. 3B 18-137
 * https://software.intel.com/en-us/download/intel-64-and-ia-32-architectures-sdm-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4
 */
static inline uint64_t
__th_get_cpu_hz_cpuid(void)
{
    unsigned int eax_denominator, ebx_numerator, ecx_crystal_hz, edx;
    eax_denominator = ebx_numerator = ecx_crystal_hz = edx = 0;

    /* CPUID 15H TSC/Crystal ratio, plus optionally Crystal Hz */
    if (!__get_cpuid(0x15, &eax_denominator, &ebx_numerator, &ecx_crystal_hz, &edx))
        return 0;

    if (eax_denominator == 0 || ebx_numerator == 0)
        return 0;

    /*
     * Some Intel SoCs like Skylake and Kabylake don't report the crystal
     * clock, but we can easily calculate it to a high degree of accuracy
     * by considering the crystal ratio and the CPU speed.
     */
    if (ecx_crystal_hz == 0)
    {
        unsigned int eax_base_mhz = 0, ebx, ecx, edx;
        if (!__get_cpuid(0x16, &eax_base_mhz, &ebx, &ecx, &edx))
            return 0;
        return eax_base_mhz * 1000000;
    }

    /* Nominal TSC frequency = ( CPUID.15H.ECX[31:0] * CPUID.15H.EBX[31:0] ) ÷ CPUID.15H.EAX[31:0] */
    return ecx_crystal_hz * ebx_numerator / eax_denominator;
}

static inline uint64_t
__th_get_cpu_hz_meas(void)
{
    unsigned int tscBegin, tscEnd, tscTemp;
    int err;
    struct timespec sleepTime = {1, 0};
    __TH_RDTSC_BEGIN(tscBegin);
    do {
        err = nanosleep(&sleepTime, &sleepTime);
    } while (err && (sleepTime.tv_sec > 0 || sleepTime.tv_nsec > 0));
    if (__th_status == TOMAHAWK_USE_RDTSCP_MEAS)
    {
        __TH_RDTSCP_END(tscEnd);
    }
    else
    {
        __TH_RDTSC_END(tscEnd);
    }
    tscTemp = tscEnd - tscBegin;
    sleepTime.tv_sec = 2;
    sleepTime.tv_nsec = 0;
    __TH_RDTSC_BEGIN(tscBegin);
    do {
        err = nanosleep(&sleepTime, &sleepTime);
    } while (err && (sleepTime.tv_sec > 0 || sleepTime.tv_nsec > 0));
    if (__th_status == TOMAHAWK_USE_RDTSCP_MEAS)
    {
        __TH_RDTSCP_END(tscEnd);
    }
    else
    {
        __TH_RDTSC_END(tscEnd);
    }
    return tscEnd - tscBegin - tscTemp;
}

static inline void
__th_set_status(void)
{
    unsigned int eax, ebx, ecx, edx;
    eax = ebx = ecx = edx = 0;
    /* Determine Invariant TSC available */
    if (!__get_cpuid(0x80000007, &eax, &ebx, &ecx, &edx) || !(edx & (1U << 8U)))
    {
        __th_status = TOMAHAWK_USE_CLOCK_GETTIME;
        return;
    }
    if (!__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx) || !(edx & (1U << 27U)))
        __th_status = TOMAHAWK_USE_RDTSC;
    else
        __th_status = TOMAHAWK_USE_RDTSCP;
    __th_cpu_hz = __th_get_cpu_hz_cpuid();
    if (__th_cpu_hz == 0)
    {
        __th_status = (__th_status == TOMAHAWK_USE_RDTSCP) ? TOMAHAWK_USE_RDTSCP_MEAS :
                                                             TOMAHAWK_USE_RDTSC_MEAS;
        __th_cpu_hz = __th_get_cpu_hz_meas();
    }
}

static inline th_error_t
th_timer_begin(th_timer *timer)
{
    switch (__th_status)
    {
        case TOMAHAWK_USE_RDTSCP:
        case TOMAHAWK_USE_RDTSCP_MEAS:
        case TOMAHAWK_USE_RDTSC:
        case TOMAHAWK_USE_RDTSC_MEAS:
            __TH_RDTSC_BEGIN(timer->begin);
            break;
        case TOMAHAWK_USE_CLOCK_GETTIME:
            clock_gettime(CLOCK_MONOTONIC, &timer->__clock_begin);
            break;
        case TOMAHAWK_NOT_USE:
        default:
            return TOMAHAWK_NOT_USE_ERROR;
    }
    return TOMAHAWK_NO_ERROR;
}

static inline th_error_t
th_timer_end(th_timer *timer)
{
    switch (__th_status)
    {
        case TOMAHAWK_USE_RDTSCP:
        case TOMAHAWK_USE_RDTSCP_MEAS:
            __TH_RDTSCP_END(timer->end);
            break;
        case TOMAHAWK_USE_RDTSC:
        case TOMAHAWK_USE_RDTSC_MEAS:
            __TH_RDTSC_END(timer->end);
            break;
        case TOMAHAWK_USE_CLOCK_GETTIME:
            clock_gettime(CLOCK_MONOTONIC, &timer->__clock_end);
            timer->begin = (timer->__clock_begin).tv_sec * __NSEC_PER_SEC + (timer->__clock_begin).tv_nsec;
            timer->end = (timer->__clock_end).tv_sec * __NSEC_PER_SEC + (timer->__clock_end).tv_nsec;
            break;
        case TOMAHAWK_NOT_USE:
        default:
            return TOMAHAWK_NOT_USE_ERROR;
    }
    if (timer->begin == 0)
        return TOMAHAWK_TIMER_NO_BEGIN_ERROR;
    return (timer->end - __th_overhead > timer->begin) ? TOMAHAWK_NO_ERROR : TOMAHAWK_TIMER_OVERFLOW_ERROR;
}

static inline th_error_t
th_timer_end_to_time(th_timer *timer, struct timespec * tp)
{
    th_error_t end_err = th_timer_end(timer);
    if (end_err)
        return end_err;

    uint64_t diff = timer->end - __th_overhead - timer->begin;
    if (__th_status != TOMAHAWK_USE_CLOCK_GETTIME)
    {
        diff = diff * __NSEC_PER_SEC / __th_cpu_hz;
    }
    tp->tv_sec = diff / __NSEC_PER_SEC;
    tp->tv_nsec = (long) (diff - tp->tv_sec * __NSEC_PER_SEC);
    return TOMAHAWK_NO_ERROR;
}

static inline uint64_t
th_timer_end_to_nsec(th_timer *timer)
{
    if (th_timer_end(timer))
        return 0;
    return timer->end - __th_overhead - timer->begin;
}

static void
__th_timer_calibrate(void)
{
    uint64_t total = 0, diff, results[__TH_CALIBRATE_TIMES];
    th_timer timer;
    if (th_timer_begin(&timer) || th_timer_end(&timer))
    {
        __th_status = TOMAHAWK_NOT_USE;
        return;
    }
    th_timer_begin(&timer);
    th_timer_end(&timer);
    th_timer_begin(&timer);
    th_timer_end(&timer);
    for (int i = 0; i < __TH_CALIBRATE_TIMES; i++)
    {
        th_timer_begin(&timer);
        results[i] = th_timer_end_to_nsec(&timer);
        total += results[i];
    }
    __th_overhead = total / __TH_CALIBRATE_TIMES;
    total = 0;
    for (int i = 0; i < __TH_CALIBRATE_TIMES; i++)
    {
        diff = results[i] > __th_overhead ? results[i] - __th_overhead :
                                            __th_overhead - results[i];
        total += diff * diff;
    }
    __th_relative_error = sqrt((double) total / (__TH_CALIBRATE_TIMES - 1)) / __th_overhead;
}


__attribute__((constructor)) static void
__th_timer_constructor(void)
{
    __th_set_status();
    __th_timer_calibrate();
}

static inline th_status_t
th_status(void)
{
    return __th_status;
}

static inline uint64_t
th_overhead(void)
{
    return __th_overhead;
}

static inline double
th_relative_error(void)
{
    return __th_relative_error;
}

#endif /* TOMAHAWK_H */