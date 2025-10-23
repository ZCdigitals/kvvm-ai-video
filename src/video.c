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

static int fd = -1;
static struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;

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

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        fprintf(stderr, "设备不支持多平面视频采集: %x\n", cap.capabilities);
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

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("设置格式失败");
        close_v4l2();
        return -1;
    }

    return 0;
}

int init_v4l2_mmap(void)
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("申请缓冲区失败");
        close_v4l2();
        return -1;
    }

    if (req.count < 2)
    {
        fprintf(stderr, "缓冲区数量不足\n");
        close_v4l2();
        return -1;
    }

    n_buffers = req.count;
    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers)
    {
        perror("分配缓冲区内存失败");
        close_v4l2();
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

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            perror("查询缓冲区失败");
            close_v4l2();
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
                perror("设置平面缓冲区失败");
                close_v4l2();
                return -1;
            }
        }
    }

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

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("队列缓冲区失败");
            close_v4l2();
            return -1;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("启动流失败");
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

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        perror("出队缓冲区失败");
        return -1;
    }

    *frame_data_y = buffers[buf.index].planes[0].start;
    *frame_size_y = buffers[buf.index].planes[0].length;
    *frame_data_uv = buffers[buf.index].planes[1].start;
    *frame_size_uv = buffers[buf.index].planes[1].length;

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

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
}

void close_v4l2(void)
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
