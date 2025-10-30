#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include <fcntl.h>

#define BUFFER_COUNT 4

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1088
#define OUTPUT_PATH "data/frame.raw"

int save_data(void *data, size_t size)
{
    FILE *f = fopen(OUTPUT_PATH, "wb");
    if (f == NULL)
    {
        perror("save data file open error");
        return -1;
    }

    fwrite(data, 1, size, f);
    fflush(f);

    fclose(f);

    return 0;
}

typedef struct
{
    void *start;
    size_t length;
} buffer_plane;

/**
 * running
 */
static volatile bool keep_running = true;

void main_stop()
{
    keep_running = false;
}

int main()
{
    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    struct v4l2_capability cap;

    int fd = open(VIDEO_PATH, O_RDWR);
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
    fmt.fmt.pix_mp.width = VIDEO_WIDTH;
    fmt.fmt.pix_mp.height = VIDEO_HEIGHT;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
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

    // init memory
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video buffers not support dma");
        return 0;
    }
    printf("video buffers support dma\n");

    if (req.count < 2)
    {
        perror("video buffers count not enough");
        return 0;
    }
    printf("video buffers count %d\n", req.count);

    buffer_plane planes[VIDEO_MAX_PLANES];
    for (unsigned int i = 0; i < req.count; i += 1)
    {
        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = 1;
        buf.m.planes = &plane;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            perror("video device qeury buffer error");
            close(fd);
            return -1;
        }

        planes[i].length = plane.length;
        planes[i].start = mmap(NULL,
                               plane.length,
                               PROT_READ | PROT_WRITE, MAP_SHARED,
                               fd,
                               plane.m.mem_offset);
        if (planes[i].start == MAP_FAILED)
        {
            perror("video device mmap error");
            close(fd);
            return -1;
        }
    }

    // start
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        perror("video device start stream error");
        return -1;
    }
    printf("video device start stream ok\n");

    while (keep_running)
    {
        // input
        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = 1;
        buf.m.planes = &plane;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
        {
            perror("video device dequeue buffer error");
            continue;
        }
        printf("video device dequeue buffer %d %d %d ok\n", buf.index, buf.sequence, plane.bytesused);

        // save data
        buffer_plane bp = planes[buf.index];
        save_data(bp.start, bp.length);

        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            perror("video device requeue buffer error");
            continue;
        }
        printf("video device requeue buffer %d ok\n", buf.index);
    }

    // stop
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
    {
        perror("video device stop stream error");
        return -1;
    }
    printf("video device stop stream ok\n");

    // close
    close(fd);
    printf("video device close ok\n");

    return 0;
}
