#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include "enc.h"
#include "socket.h"
#include "video.h"

#define BIT_RATE 10 * 1024
#define GOP 2

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture.sock"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

void main_stop()
{
    keep_running = 0;
}

void *input(void *)
{
    void *dst = use_venc_frame();
    if (dst == NULL)
    {
        errno = -1;
        perror("null destnation");
        return NULL;
    }

    int ret = capture_v4l2_frame_single_buffer(dst);
    if (ret == -1)
    {
        return NULL;
    }

    ret = send_venc_frame(VIDEO_WIDTH, VIDEO_WIDTH, !keep_running);
    if (ret == -1)
    {
        return NULL;
    }

    return NULL;
}

void *output(void *)
{
    void *frame;
    unsigned int size;

    int ret = read_venc_frame(&frame, &size);
    if (ret == -1)
    {
        return NULL;
    }

    ret = send_frame(VIDEO_WIDTH, VIDEO_WIDTH, frame, size);
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
    int ret = init_venc(BIT_RATE, GOP, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        errno = ret;
        perror("init venc error");
        return -1;
    }

    ret = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        errno = ret;
        perror("init v4l2 error");
        close_venc();
        return -1;
    }

    ret = init_socket(OUTPUT_PATH);
    if (ret == -1)
    {
        errno = ret;
        perror("init socket error");
        close_v4l2();
        close_venc();
        return -1;
    }

    return 0;
}

int start()
{
    int ret = start_venc();
    if (ret == -1)
    {
        errno = ret;
        perror("start venc error");
        return -1;
    }

    ret = start_v4l2_capture();
    if (ret == -1)
    {
        errno = ret;
        perror("start v4l2 error");
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
