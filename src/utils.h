#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include <sys/time.h>

#define US_PER_SECOND 1000000

/**
 * time to us
 *
 * @param tv time
 * @return time in us
 */
uint64_t time_to_us(struct timeval tv);

/**
 * get time in us
 *
 * @return time in us
 */
uint64_t get_time_us();

#endif
