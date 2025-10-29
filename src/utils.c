#include <stdlib.h>
#include <sys/time.h>

#define US_PER_SECOND 1000000

unsigned long long int sec_to_us(struct timeval *tv)
{
    return tv->tv_sec * US_PER_SECOND + tv->tv_usec;
}

unsigned long long int get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return sec_to_us(&tv);
}
