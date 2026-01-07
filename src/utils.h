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

/**
 * calculate picture byte size
 *
 * @param width picture width
 * @param height picture height
 * @param pic_format v4l2 picture color format
 * @return byte size, -1 is error
 */
uint32_t calculate_pic_byte_size(uint32_t width, uint32_t height, uint32_t pic_format);

#endif
