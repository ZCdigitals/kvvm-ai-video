#include <stdio.h>

#include "video.h"

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "data/frame.raw"

int save_data_nv12(const char *path, void *data_y, size_t size_y, void *data_uv, size_t size_uv)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        perror("file open error");
        return -1;
    }

    fwrite(data_y, 1, size_y, fp);
    fwrite(data_uv, 1, size_uv, fp);

    fclose(fp);
    printf("file save %s\n", path);
    return 0;
}

int main()
{
    void *frame_data_y;
    size_t frame_size_y;
    void *frame_data_uv;
    size_t frame_size_uv;

    // init
    if (init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT) < 0)
    {
        return -1;
    }
    // start
    if (start_v4l2_capture() < 0)
    {
        close_v4l2();
        return -2;
    }

    // capture
    unsigned int buffer_index = capture_v4l2_frame(&frame_data_y, &frame_size_y, &frame_data_uv, &frame_size_uv);

    if (buffer_index != -1)
    {
        // save file
        save_data_nv12(OUTPUT_PATH, frame_data_y, frame_size_y, frame_data_uv, frame_size_uv);
    }
    else
    {
        printf("null buffer\n");
    }

    // stop
    stop_v4l2_capture();

    // end
    close_v4l2();

    return 0;
}
