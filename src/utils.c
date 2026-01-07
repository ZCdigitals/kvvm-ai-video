#include "utils.h"

#include <stdlib.h>

#include <linux/videodev2.h>

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

uint32_t calculate_pic_byte_size(uint32_t width, uint32_t height, uint32_t pic_format)
{
    switch (pic_format)
    {
    case V4L2_PIX_FMT_NV12:
        return width * height * 3 / 2;
    default:
        return -1;
    }
}
