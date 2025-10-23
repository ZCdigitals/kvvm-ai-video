#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "video.h"
#include "file.h"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

static const char *VIDEO_PATH = "/dev/video0";
static const char *OUTPUT_PATH = "/tmp/capture";

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

    // v4l2
    if (init_v4l2(VIDEO_PATH) < 0)
    {
        return 1;
    }
    if (setup_v4l2(1920, 1080) < 0)
    {
        return 2;
    }

    if (init_v4l2_mmap() < 0)
    {
        return 3;
    }

    if (start_v4l2_capture() < 0)
    {
        return 4;
    }

    while (keep_running)
    {
        int buffer_index = capture_v4l2_frame(&frame_data_y, &frame_size_y, &frame_data_uv, &frame_size_uv);
        if (buffer_index >= 0)
        {
            if (save_data_nv12(OUTPUT_PATH, frame_data_y, frame_size_y, frame_data_uv, frame_size_uv) == 0)
            {
                printf("成功捕获");
            }
        }

        usleep(100000);

        main_stop();
    }

    stop_v4l2_capture();

    close_v4l2();

    return 0;
}
