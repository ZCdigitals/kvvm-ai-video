#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "enc.h"
#include "socket.h"
#include "utils.h"
#include "video.h"

#define BIT_RATE 10 * 1024
#define GOP 2

#define BUFFER_COUNT 4

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture.sock"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

struct buffer
{
    int fd;
};

// buffers
static struct buffer *buffers = NULL;
static unsigned int buffer_count = 0;

void main_stop()
{
    keep_running = 0;
}

void *input(void *args)
{
    unsigned int id = 0;
    unsigned long long int time = 0;

    // capture frame, dequeue buffer
    int plane_fd = capture_v4l2_frame(&id, &time);
    if (plane_fd == -1)
    {
        return NULL;
    }

    // send to enc
    int ret = send_venc_frame(plane_fd, id, time, !keep_running);
    if (ret == -1)
    {
        return NULL;
    }

    // release frame, requeue buffer
    ret = release_v4l2_frame();
    if (ret == -1)
    {
        return NULL;
    }

    return NULL;
}

static volatile uint32_t frame_id = 0;

void *output(void *args)
{
    void *frame;
    unsigned int size;

    int ret = read_venc_frame(&frame, &size);
    if (ret == -1)
    {
        return NULL;
    }

    ret = send_frame(frame_id, get_time_us(), frame, size);
    frame_id += 1;
    if (ret == -1)
    {
        return NULL;
    }

    ret = release_venc_frame();
    if (ret == -1)
    {
        return NULL;
    }

    return NULL;
}

int init()
{
    buffer_count = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT, BUFFER_COUNT);
    if (buffer_count == 0)
    {
        return -1;
    }

    int ret = init_venc(BIT_RATE, GOP, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        close_v4l2();
        goto error1;
    }

    ret = init_socket(OUTPUT_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        close_v4l2();
        close_venc();
        goto error2;
    }

    buffers = calloc(buffer_count, sizeof(*buffers));
    if (buffers == NULL)
    {
        errno = -1;
        perror("calloc buffers error");
        goto error3;
    }

    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        int fd = use_venc_frame();
        if (fd == -1)
        {
            goto error3;
        }
        buffers[i].fd = fd;

        ret = init_v4l2_buffer(i, fd);
        if (ret == -1)
        {
            goto error3;
        }
    }

error3:
    close_socket();
    goto error2;
error2:
    close_venc();
    goto error1;
error1:
    close_v4l2();
    return 0;
}

int start()
{
    int ret = start_venc();
    if (ret == -1)
    {
        return -1;
    }

    ret = start_v4l2_capture();
    if (ret == -1)
    {
        return -1;
    }

    return 0;
}

int stop()
{
    stop_v4l2_capture();
    stop_venc();
    return 0;
}

int close()
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        free_venc_frame(buffers[i].fd);
    }

    close_v4l2();
    close_venc();
    close_socket();

    return 0;
}

int main()
{
    int ret = init();
    if (ret == -1)
    {
        return -1;
    }

    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    ret = start();
    if (ret == -1)
    {
        close();
        return -1;
    }

    while (keep_running)
    {
    }

    stop();
    close();

    return 0;
}
