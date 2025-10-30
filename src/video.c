#include "video.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include <unistd.h>
#include <fcntl.h>

#include "utils.h"

static int fd = -1;

unsigned int init_v4l2(const char *path, int width, int height, unsigned int buffer_count)
{
    struct v4l2_capability cap;

    fd = open(path, O_RDWR);
    if (fd == -1)
    {
        perror("video device open error");
        return 0;
    }
    printf("video device open ok\n");

    // check device capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        perror("video device query capabilities error");
        close(fd);
        return 0;
    }
    printf("video device query capabilities ok\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        perror("video device not support capture multiple plane");
        close(fd);
        return 0;
    }
    printf("video device support capture multiple plane\n");

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        perror("video device not support streaming");
        close(fd);
        return 0;
    }
    printf("video device support streaming\n");

    // setup start
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    // fmt.fmt.pix_mp.num_planes = 1;

    // fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    // fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height;
    // fmt.fmt.pix_mp.plane_fmt[1].bytesperline = width;
    // fmt.fmt.pix_mp.plane_fmt[1].sizeimage = width * height / 2;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("video device set format error");
        close(fd);
        return 0;
    }
    printf("video device set format ok\n");
    // setup end

    // init buffers start
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        close(fd);
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video buffers not support dma");
        close(fd);
        return 0;
    }
    printf("video buffers support dma\n");

    if (req.count < 2)
    {
        perror("video buffers count not enough");
        close(fd);
        return 0;
    }
    printf("video buffers count %d\n", req.count);

    return req.count;
}

static struct v4l2_buffer vbuf;
static struct v4l2_plane vplane;

unsigned int init_v4l2_buffer(unsigned int index)
{
    memset(&vbuf, 0, sizeof(vbuf));
    memset(&vplane, 0, sizeof(vplane));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    vbuf.memory = V4L2_MEMORY_DMABUF;
    vbuf.index = index;
    vbuf.length = 1;
    vbuf.m.planes = &vplane;

    if (ioctl(fd, VIDIOC_QUERYBUF, &vbuf) == -1)
    {
        perror("video device query buffer error");
        close(fd);
        return -1;
    }
    printf("video device query buffer ok\n");

    return vplane.length;
}

int use_v4l2_buffer(int plane_fd)
{
    vplane.m.fd = plane_fd;

    if (ioctl(fd, VIDIOC_QBUF, &vbuf) == -1)
    {
        perror("video device queue buffer error");
        return -1;
    }
    printf("video device queue buffer ok\n");

    return 0;
}

int start_v4l2_capture()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        perror("video device start streaming error");
        return -1;
    }
    printf("video device start streaming ok\n");

    return 0;
}

static volatile struct v4l2_buffer *buffer = NULL;

int capture_v4l2_frame(unsigned int *id, unsigned long long int *time)
{
    struct v4l2_buffer buf;
    struct v4l2_plane plane;

    memset(&buf, 0, sizeof(buf));
    memset(&plane, 0, sizeof(plane));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.length = 1;
    buf.m.planes = &plane;
    // plane.m.fd = plane_fd;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
        perror("video device dequeue buffer error");
        return -1;
    }
    printf("video device dequeue buffer ok\n");

    buffer = &buf;
    *id = buf.sequence;
    *time = time_to_us(buf.timestamp);

    return plane.m.fd;
}

int release_v4l2_frame()
{
    if (buffer == NULL)
    {
        errno = -1;
        perror("null buffer, call `capture_v4l2_frame` first");
        return -1;
    }

    if (ioctl(fd, VIDIOC_QBUF, buffer) == -1)
    {
        perror("video device queue buffer error");
        close(fd);
        return -1;
    }
    printf("video device queue buffer ok\n");
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
    printf("video device stop stream ok\n");

    return 0;
}

void close_v4l2()
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
        printf("video device close ok\n");
    }
}
