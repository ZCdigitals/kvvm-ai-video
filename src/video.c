#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "video.h"

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

static volatile int fd = -1;
static volatile struct buffer *buffers = NULL;
static volatile unsigned int n_buffers = 0;

int init_v4l2(const char *path, int width, int height)
{
    struct v4l2_capability cap;

    fd = open(path, O_RDWR);
    if (fd == -1)
    {
        perror("video device open error");
        return -1;
    }

    // check device capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        perror("video device query capabilities error");
        close(fd);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        perror("video device not support multi plane");
        close(fd);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        perror("video device not support streaming");
        close(fd);
        return -1;
    }

    // setup start
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 2;

    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height;
    fmt.fmt.pix_mp.plane_fmt[1].bytesperline = width;
    fmt.fmt.pix_mp.plane_fmt[1].sizeimage = width * height / 2;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("video device set format error");
        close(fd);
        return -1;
    }
    // setup end

    // init mmap start
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        close(fd);
        return -1;
    }

    if (req.count < 2)
    {
        perror("video device buffer count not enough");
        close(fd);
        return -1;
    }

    n_buffers = req.count;
    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers)
    {
        perror("video device calloc buffers error");
        close(fd);
        return -1;
    }

    for (unsigned int i = 0; i < n_buffers; i++)
    {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[VIDEO_MAX_PLANES];

        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = VIDEO_MAX_PLANES;
        buf.m.planes = planes;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            perror("video device query buffer error");
            close(fd);
            return -1;
        }

        buffers[i].n_planes = buf.length;

        for (unsigned int j = 0; j < buf.length; j++)
        {
            buffers[i].planes[j].length = buf.m.planes[j].length;
            buffers[i].planes[j].start = mmap(NULL, buf.m.planes[j].length,
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                              fd, buf.m.planes[j].m.mem_offset);
            if (MAP_FAILED == buffers[i].planes[j].start)
            {
                perror("video device set plane buffer error");
                close(fd);
                return -1;
            }
        }
    }
    // init mmap end

    return 0;
}

int start_v4l2_capture(void)
{

    for (unsigned int i = 0; i < n_buffers; i++)
    {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[VIDEO_MAX_PLANES];

        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = buffers[i].n_planes;
        buf.m.planes = planes;

        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            perror("video device query buffer error");
            return -1;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        perror("video device start streaming error");
        return -1;
    }

    return 0;
}

int capture_v4l2_frame(void **frame_data_y, size_t *frame_size_y, void **frame_data_uv, size_t *frame_size_uv)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
        perror("video device dequeue buffer error");
        return -1;
    }

    *frame_data_y = buffers[buf.index].planes[0].start;
    *frame_size_y = buffers[buf.index].planes[0].length;
    *frame_data_uv = buffers[buf.index].planes[1].start;
    *frame_size_uv = buffers[buf.index].planes[1].length;

    // add to queue after processing
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
        perror("video device requeue buffer error");
        return -1;
    }

    return buf.index;
}

int capture_v4l2_frame_single_buffer(void *dst)
{
    void *frame_data_y;
    size_t frame_size_y;
    void *frame_data_uv;
    size_t frame_size_uv;

    int ret = capture_v4l2_frame(&frame_data_y, &frame_size_y, &frame_data_uv, &frame_size_uv);
    if (ret == -1)
    {
        return -1;
    }

    // todo, maybe could be zero copy
    // for now, multiple planes must copy memory

    memcpy(dst, frame_data_y, frame_size_y);
    memcpy((char *)dst + frame_size_y, frame_data_uv, frame_size_uv);

    return 0;
}

int stop_v4l2_capture()
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
    {
        perror("video device stop stream error");
        return -1;
    }

    return 0;
}

void close_v4l2()
{
    if (buffers)
    {
        for (unsigned int i = 0; i < n_buffers; i++)
        {
            for (unsigned int j = 0; j < buffers[i].n_planes; j++)
            {
                if (buffers[i].planes[j].start != MAP_FAILED)
                {
                    munmap(buffers[i].planes[j].start, buffers[i].planes[j].length);
                }
            }
        }
        free(buffers);
        buffers = NULL;
        n_buffers = 0;
    }

    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}
