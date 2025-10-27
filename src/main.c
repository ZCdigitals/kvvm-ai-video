#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "socket.h"
#include "video.h"

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

void main_stop()
{
    keep_running = 0;
}

int main()
{
    void *frame_data_y;
    size_t frame_size_y;
    void *frame_data_uv;
    size_t frame_size_uv;

    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    // init
    if (init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT) < 0)
    {
        return -1;
    }
    if (init_socket(OUTPUT_PATH) < 0)
    {
        close_socket();
        return -1;
    }

    // start
    if (start_v4l2_capture() < 0)
    {
        return -2;
    }

    while (keep_running)
    {
        int buffer_index = capture_v4l2_frame(&frame_data_y, &frame_size_y, &frame_data_uv, &frame_size_uv);
        if (buffer_index >= 0)
        {
            send_socket(VIDEO_WIDTH, VIDEO_HEIGHT, frame_data_y, frame_size_y);
        }

        usleep(100000);

        // main_stop();
    }

    // stop
    stop_v4l2_capture();

    // end
    close_v4l2();

    return 0;
}
