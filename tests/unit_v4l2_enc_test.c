#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "enc.h"
#include "video.h"

#define BIT_RATE 10 * 1024
#define GOP 2

#define BUFFER_COUNT 4

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "data/frame.h264"

struct buffer
{
    int fd;
};

// buffers
static struct buffer *buffers = NULL;
static unsigned int buffer_count = 0;

int save_data(const char *path, void *data, size_t size)
{

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        perror("file open error");
        return -1;
    }

    fwrite(data, 1, size, fp);

    fclose(fp);
    printf("file save %s\n", path);
    return 0;
}

int main()
{
    // init
    buffer_count = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT, BUFFER_COUNT);
    if (buffer_count == 0)
    {
        return -1;
    }

    int ret = init_venc(BIT_RATE, GOP, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        return -1;
    }

    buffers = calloc(buffer_count, sizeof(*buffers));
    if (buffers == NULL)
    {
        errno = -1;
        perror("calloc buffers error");
        return -1;
    }

    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        int fd = use_venc_frame();
        if (fd == -1)
        {
            return -1;
        }
        buffers[i].fd = fd;

        ret = init_v4l2_buffer(i, fd);
        if (ret == -1)
        {
            return -1;
        }
    }

    // start
    ret = start_venc();
    if (ret == -1)
    {
        return -1;
    }

    ret = start_v4l2_capture();
    if (ret == -1)
    {
        return -1;
    }

    // input
    unsigned int id = 0;
    unsigned long long int time = 0;
    // capture frame, dequeue buffer
    int plane_fd = capture_v4l2_frame(&id, &time);
    if (plane_fd == -1)
    {
        return -1;
    }

    // send to enc
    ret = send_venc_frame(plane_fd, id, time, true);
    if (ret == -1)
    {
        return -1;
    }

    // release frame, requeue buffer
    ret = release_v4l2_frame();
    if (ret == -1)
    {
        return -1;
    }

    // output
    void *frame;
    unsigned int size;

    ret = read_venc_frame(&frame, &size);
    if (ret == -1)
    {
        return -1;
    }

    ret = save_data(OUTPUT_PATH, frame, size);
    if (ret == -1)
    {
        return -1;
    }

    ret = release_venc_frame();
    if (ret == -1)
    {
        return -1;
    }

    // stop
    stop_v4l2_capture();
    stop_venc();

    // close
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        free_venc_frame(buffers[i].fd);
    }

    close_v4l2();
    close_venc();

    return 0;
}
