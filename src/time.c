#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

uint64_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}
