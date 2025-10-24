#ifndef BUFFER_H
#define BUFFER_H

#include <linux/videodev2.h>
#include <stddef.h>

#define BUFFER_COUNT 4

struct buffer_plane
{
    void *start;
    size_t length;
};

struct buffer
{
    struct buffer_plane planes[VIDEO_MAX_PLANES];
    int n_planes;
};

#endif /* BUFFER_H */
