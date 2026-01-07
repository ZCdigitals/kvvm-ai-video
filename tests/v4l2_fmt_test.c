#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <fcntl.h>
#include <unistd.h>

#define BUFFER_COUNT 4

int init_v4l2(const char *path, unsigned int width, unsigned int height)
{
    struct v4l2_capability cap;

    int fd = open(path, O_RDWR);
    if (fd == -1)
    {
        perror("video device open error");
        return -1;
    }
    printf("video device open ok\n");

    // check device capabilities
    int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret == -1)
    {
        perror("video device query capabilities error");
        close(fd);
        return -1;
    }
    printf("video device query capabilities ok\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        perror("video device not support capture multiple plane");
        close(fd);
        return -1;
    }
    printf("video device support capture multiple plane\n");

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        perror("video device not support streaming");
        close(fd);
        return -1;
    }
    printf("video device support streaming\n");

    // set format
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 1;

    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 3 / 2;

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret == -1)
    {
        perror("video device set format error");
        close(fd);
        return -1;
    }
    printf("video device set format ok\n");

    return fd;
}

unsigned int init_v4l2_buffers(int fd, unsigned int buffer_count)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video device not support dma");
        return 0;
    }
    printf("video device support dma\n");

    if (req.count < 2)
    {
        perror("video device buffers count not enough");
        return 0;
    }
    printf("video device buffers count %d\n", req.count);

    return req.count;
}

int allocate_buffers(int video_fd, unsigned int buffer_count)
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // use v4l2 buffer
        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.index = i;
        buf.length = 1;
        buf.m.planes = &plane;

        // although we use dma, query buffer is also necessary, this will init plane
        int ret = ioctl(video_fd, VIDIOC_QUERYBUF, &buf);
        if (ret == -1)
        {
            perror("video device query buffer error");
            return -1;
        }
        printf("video device query buffer %d ok %d\n", i, plane.length);
    }
}

void destroy_v4l2(int fd)
{
    int ret = close(fd);
    if (ret == -1)
    {
        perror("video device close error");
    }
    printf("video device close ok\n");
}

int main()
{
    int fd = init_v4l2("/dev/video0", 1920, 1080);
    init_v4l2_buffers(fd, BUFFER_COUNT);
    allocate_buffers(fd, BUFFER_COUNT);
    destroy_v4l2(fd);
}
