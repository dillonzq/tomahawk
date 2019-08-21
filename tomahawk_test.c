/**
 * Author: Dillon <dillonzq@outlook.com>
 */
#include "tomahawk.h"
#include <stdio.h>
#include <time.h>

int
main(void)
{
    th_timer timer;
    struct timespec result, sleepTime = {1, 0};
    int err;

    printf("TH Status: %i\n\n", th_status());
    printf("TH Overhead: %llu\n", th_overhead());
    printf("TH Relative Error: %f %%\n\n", th_relative_error());

    if (th_timer_begin(&timer))
        return -1;
    do {
        err = nanosleep(&sleepTime, &sleepTime);
    } while (err && (sleepTime.tv_sec > 0 || sleepTime.tv_nsec > 0));
    if (th_timer_end_to_time(&timer, &result))
        return -1;

    printf("Reslut: %ld sec %ld nsec\n", result.tv_sec, result.tv_nsec);
    return 0;
}