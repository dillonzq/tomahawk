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
    th_recorder recorder = {NULL, 100, 0};
    unsigned count;

    printf("TH Status: %i\n\n", th_status());
    printf("TH Overhead: %llu\n", th_overhead());
    printf("TH Relative Error: %f %%\n\n", th_relative_error());

    for(int j = 0; j < 1000; j++)
    {
        if (th_timer_begin(&timer))
            return -1;
        for (int i = 0; i < 1000; i++);
        if (th_timer_end_to_recorder(&timer, &recorder))
            return -1;
        if (recorder.len >= 100)
        {
            count = th_flush_recorder(&recorder);
            printf("Count: %u, Total: %llu, Average: %llu, Error: %f %%\n\n", count, recorder.total, recorder.average, recorder.error);
        }
    }
    return 0;
}
