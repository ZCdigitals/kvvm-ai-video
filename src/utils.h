#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>

#define US_PER_SECOND 1000000

/**
 * time to us
 *
 * @param tv time
 * @return time in us
 */
unsigned long long int time_to_us(struct timeval tv);

/**
 * get time in us
 *
 * @return time in us
 */
unsigned long long int get_time_us();

#endif
