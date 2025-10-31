#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/time.h>

#include <unistd.h>

#include "socket.h"
#include "utils.h"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080

#define VIDEO_PATH "data/frame.h264"

#define OUTPUT_PATH "/tmp/capture.sock"

#define FRAME_DURATION 33333

// running
static volatile int keep_running = 1;

/**
 * stop running
 */
void stop_running()
{
    keep_running = 0;
}

static int frame_id = 0;

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
    // regist signal
    signal(SIGINT, stop_running);
    signal(SIGTERM, stop_running);

    unsigned char *buffer;

    size_t size = read_file(&buffer);
    if (size == -1)
    {
        return -1;
    }

    int fd = init_socket(OUTPUT_PATH);
    if (fd == -1)
    {
        return -1;
    }

    while (keep_running)
    {
        int ret = send_frame(fd, frame_id, get_time_us(), VIDEO_WIDTH, VIDEO_HEIGHT, buffer, size);
        if (ret == -1)
        {
            stop_running();
        }
        else
        {
            usleep(FRAME_DURATION);
        }

        frame_id += 1;
    }

    destroy_socket(fd);
    free(buffer);

    return 0;
}
