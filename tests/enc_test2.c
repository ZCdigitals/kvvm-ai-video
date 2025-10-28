#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "enc.h"

#define INPUT_PATH "data/frame.raw"
#define OUPUT_PATH "data/frame.h264"

#define BIT_RATE 10 * 1024
#define GOP 60
#define FRAME_COUNT 1
#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080

int read_image_nv12(void *dst, uint32_t width, uint32_t height, uint32_t vir_width, uint32_t vir_height, FILE *f)
{
    uint32_t row = 0;
    uint32_t size = 0;

    uint8_t *buffer_y = dst;
    uint8_t *buffer_uv = buffer_y + vir_width * vir_height;

    for (row = 0; row < vir_height; row++)
    {
        size = fread(buffer_y + row * vir_width, 1, width, f);
        if (size != width)
        {
            return -1;
        }
    }

    for (row = 0; row < vir_height / 2; row++)
    {
        size = fread(buffer_uv + row * vir_width, 1, width, f);
        if (size != width)
        {
            return -1;
        }
    }

    return 0;
}

int write_image(void *src, unsigned int size, FILE *f)
{
    fwrite(src, 1, size, f);
    fflush(f);
}

void *input()
{
    void *dst = use_venc_frame();
    if (dst == NULL)
    {
        errno = -1;
        perror("null dst");
        return NULL;
    }

    FILE *f = fopen(INPUT_PATH, "r");
    read_image_nv12(dst, FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT, f);
    fclose(f);

    send_venc_frame(FRAME_WIDTH, FRAME_HEIGHT, true);

    return NULL;
}

int output()
{
    void *data = NULL;
    unsigned int size;

    int ret = read_venc_frame(&data, &size);
    if (ret == -1)
    {
        return -1;
    }

    FILE *d = fopen(OUPUT_PATH, "wb");
    write_image(data, size, d);
    fclose(d);

    return release_venc_frame();
}

int main()
{
    int ret = init_venc(BIT_RATE, GOP, FRAME_WIDTH, FRAME_HEIGHT);
    if (ret == -1)
    {
        return -1;
    }

    ret = start_venc();
    if (ret == -1)
    {
        return -1;
    }

    pthread_t input_thread;

    // read in othrer thread
    pthread_create(&input_thread, 0, input, NULL);

    int running = 1;
    while (running)
    {
        running = !output();
    }

    pthread_join(input_thread, NULL);

    stop_venc();
    close_venc();

    return 0;
}
