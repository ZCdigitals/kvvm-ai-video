#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define BUFFER_COUNT 4

struct buffer
{
    void *start;
    size_t length;
};

static struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static int fd = -1;

int init_v4l2(const char *path)
{
    struct v4l2_capability cap;

    fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("Open video device error");
        return -1;
    }

    // 检查设备能力
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("查询设备能力失败");
        close_v4l2();
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "设备不支持视频采集\n");
        close_v4l2();
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "设备不支持流式IO\n");
        close_v4l2();
        return -1;
    }

    printf("设备: %s\n", cap.card);
    printf("驱动: %s\n", cap.driver);
    printf("总线信息: %s\n", cap.bus_info);

    return fd;
}

int setup_v4l2(int width, int height)
{
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("设置格式失败");
        return -1;
    }

    return 0;
}

int init_v4l2_mmap(void)
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("申请缓冲区失败");
        return -1;
    }

    if (req.count < 2)
    {
        fprintf(stderr, "缓冲区数量不足\n");
        return -1;
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers)
    {
        perror("分配缓冲区内存失败");
        return -1;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            perror("查询缓冲区失败");
            return -1;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length,
                                        PROT_READ | PROT_WRITE, MAP_SHARED,
                                        fd, buf.m.offset);

        if (buffers[n_buffers].start == MAP_FAILED)
        {
            perror("内存映射失败");
            return -1;
        }
    }

    return 0;
}

int start_v4l2_capture(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i)
    {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("队列缓冲区失败");
            return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("启动流失败");
        return -1;
    }
}

int capture_v4l2_frame(void **frame_data, size_t *frame_size)
{
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        perror("出队缓冲区失败");
        return -1;
    }

    *frame_data = buffers[buf.index].start;
    *frame_size = buf.bytesused;

    // 将处理后的缓冲区重新加入队列
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
    {
        perror("重新队列缓冲区失败");
        return -1;
    }

    return buf.index;
}

void stop_v4l2_capture(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
}

void close_v4l2(void)
{
    unsigned int i;

    if (buffers)
    {
        for (i = 0; i < n_buffers; ++i)
        {
            if (buffers[i].start && buffers[i].start != MAP_FAILED)
            {
                munmap(buffers[i].start, buffers[i].length);
            }
        }
        free(buffers);
    }

    if (fd >= 0)
    {
        close(fd);
    }
}
