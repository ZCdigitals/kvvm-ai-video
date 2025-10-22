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

static const char *VIDEO_PATH = "/dev/device";
static const char *OUTPUT_PATH = "/tmp/capture";

void main_stop()
{
    keep_running = 0;
}

int main()
{
    void *frame_data;
    size_t frame_size;

    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    // v4l2
    init_v4l2(VIDEO_PATH);
    init_v4l2_mmap();

    start_v4l2_capture();

    while (keep_running)
    {
        int buffer_index = capture_v4l2_frame(&frame_data, &frame_size);
        if (buffer_index >= 0)
        {
            if (save_data(OUTPUT_PATH, frame_data, frame_size) == 0)
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
