#include "utils.h"

#include <stdlib.h>

#define US_PER_SECOND 1000000

uint64_t time_to_us(struct timeval tv)
{
    return tv.tv_sec * US_PER_SECOND + tv.tv_usec;
}

uint64_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return time_to_us(tv);
}
