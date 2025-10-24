#include <stdio.h>
#include <linux/videodev2.h>

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
