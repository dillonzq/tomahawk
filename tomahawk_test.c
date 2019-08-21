/**
 * Author: Dillon <dillonzq@outlook.com>
 */
#include "tomahawk.h"
#include <stdio.h>

int
main(void)
{
    volatile th_timer timer;
    double reslut;

    if (!th_status()) {
        return -1;
    }
    printf("TH Status: %i\n\n", th_status());

    printf("TH Overhead: %llu\n", th_overhead());
    printf("TH Relative Error: %f %%\n\n", th_relative_error());

    th_timer_begin(&timer);
    reslut = th_timer_end_to_us(&timer);
    printf("Reslut: %f\n", reslut);

    return 0;
}