#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// #include <pthread.h>

#include <linux/videodev2.h>

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
    int ret = send_venc_frame(plane_fd, id, time, true);
    // if (ret == -1)
    // {
    //     return NULL;
    // }

    // release frame, requeue buffer
    ret = release_v4l2_frame();
    if (ret == -1)
    {
        return NULL;
    }

    return NULL;
}

void *output(void *args)
{
    void *frame;
    unsigned int size;

    int ret = read_venc_frame(&frame, &size);
    if (ret == -1)
    {
        return NULL;
    }

    ret = save_data(OUTPUT_PATH, frame, size);
    // if (ret == -1)
    // {
    //     return NULL;
    // }

    ret = release_venc_frame();
    if (ret == -1)
    {
        return NULL;
    }

    return NULL;
}

int init()
{
    printf("start init\n");
    buffer_count = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT, BUFFER_COUNT);
    if (buffer_count == 0)
    {
        return -1;
    }
    printf("init v4l2 ok\n");

    int ret = init_venc(BIT_RATE, GOP, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (ret == -1)
    {
        return -1;
    }
    printf("init venc ok\n");

    buffers = calloc(buffer_count, sizeof(*buffers));
    if (buffers == NULL)
    {
        errno = -1;
        perror("calloc buffers error");
        return -1;
    }

    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        unsigned int plane_length = init_v4l2_buffer(i);
        if (plane_length == 0)
        {
            return -1;
        }

        int fd = use_venc_frame(plane_length);
        if (fd == -1)
        {
            return -1;
        }
        buffers[i].fd = fd;

        int ret = use_v4l2_buffer(fd);
        if (ret == -1)
        {
            return -1;
        }
    }

    return 0;
}

int close()
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        free_venc_frame(buffers[i].fd);
    }

    free(buffers);

    close_v4l2();
    close_venc();

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

int main()
{
    // int ret = init();
    // if (ret == -1)
    // {
    //     return -1;
    // }

    // ret = start();
    // if (ret == -1)
    // {
    //     close();
    //     return -1;
    // }

    // pthread_t input_thread;
    // pthread_t output_thread;

    // ret = pthread_create(&input_thread, NULL, input, NULL);
    // if (!ret != 0)
    // {
    //     perror("input thread create error");
    //     stop();
    //     close();
    //     return -1;
    // }

    // ret = pthread_create(&output_thread, NULL, output, NULL);
    // if (!ret != 0)
    // {
    //     perror("output thread create error");
    //     pthread_join(input_thread, NULL);
    //     stop();
    //     close();
    //     return -1;
    // }

    // pthread_join(input_thread, NULL);
    // pthread_join(output_thread, NULL);

    // stop();
    // close();

    printf("main");

    return 0;
}
