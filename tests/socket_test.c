#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "socket.h"
#include "utils.h"

#define VIDEO_PATH "data/frame.h264"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture.sock"

#define FRAME_DURATION 33333

// 全局标志位，用于控制主循环
static int keep_running = 1;
static int frame_id = 0;

void main_stop()
{
    keep_running = 0;
}

int read_file(unsigned char **buffer)
{
    FILE *fd = fopen(VIDEO_PATH, "rb");
    if (fd == NULL)
    {
        perror("read file error");
        return -1;
    }

    fseek(fd, 0, SEEK_END);
    int size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    unsigned char *buf = (unsigned char *)malloc(size);
    if (buf == NULL)
    {
        perror("malloc error");
        fclose(fd);
        return -1;
    }
    size_t read = fread(buf, 1, size, fd);

    if (read != size)
    {
        perror("read error");
        fclose(fd);
        free(buf);
        return -1;
    }

    *buffer = buf;

    fclose(fd);
    return size;
}

int main()
{
    unsigned char **buffer;

    size_t size = read_file(buffer);
    if (size == -1)
    {
        return -1;
    }

    int ret = init_socket(OUTPUT_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        return -1;
    }

    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    while (keep_running)
    {
        ret = send_frame(frame_id, get_time_us(), *buffer, size);
        if (ret == -1)
        {
            main_stop();
        }
        else
        {
            usleep(FRAME_DURATION);
        }
    }

    close_socket();

    return 0;
}
