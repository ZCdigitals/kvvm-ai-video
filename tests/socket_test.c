#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "socket.h"

#define VIDEO_PATH "data/frame.h264"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

void main_stop()
{
    keep_running = 0;
}

int read_file(unsigned char **buffer)
{
    FILE *fd = fopen(VIDEO_PATH, "rb");
    if (fd < 0)
    {
        perror("read file error");
        return -1;
    }

    fseek(fd, 0, SEEK_END);
    int size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    unsigned char *buf = (unsigned char *)malloc(size);
    if (buf)
    {
        perror("malloc error");
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

    int ret = init_socket(OUTPUT_PATH);
    if (ret < 0)
    {
        fprintf(stderr, "init error %x\n", ret);
        free(buffer);
        return -1;
    }

    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    while (keep_running)
    {
        send_socket(VIDEO_WIDTH, VIDEO_HEIGHT, *buffer, size);
        usleep(33333);
    }

    ret = close_socket();
    if (ret < 0)
    {
        fprintf(stderr, "close error %x\n", ret);
    }

    free(buffer);

    return 0;
}
