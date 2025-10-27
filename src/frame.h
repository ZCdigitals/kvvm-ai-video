#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>

typedef struct
{
    uint32_t id;
    uint32_t width;
    uint32_t height;
    /**
     * - 0, nv12
     *
     * only support nv12
     */
    uint32_t format;
    // in ms
    uint64_t time;
    uint32_t size;
    uint32_t reserved;
} frame_header;

#endif
