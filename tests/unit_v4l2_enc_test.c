#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <pthread.h>

#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_venc.h"

#include "utils.h"
#include "video.h"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080

#define VENC_CHANNEL 0
#define BIT_RATE 10 * 1024
#define GOP 60
// venc send frame timeout, venc get stream timeout
#define TIMEOUT 1000

// video device path
#define VIDEO_PATH "/dev/video0"
#define BUFFER_COUNT 4

#define OUTPUT_PATH "data/frame.h264"

int save_data(void *data, unsigned int size)
{
    FILE *f = fopen(OUTPUT_PATH, "wb");
    if (f == NULL)
    {
        perror("save data file open error");
        return -1;
    }

    fwrite(data, 1, size, f);
    fflush(f);

    fclose(f);

    return 0;
}

// running
static volatile int keep_running = 1;

/**
 * stop running
 */
void stop_running()
{
    keep_running = 0;
}

/**
 * input single time
 *
 * @param venc_channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param video_fd device file description
 * @param width video width, eg. `1920`
 * @param height video height, eg. `1080`
 * @param vir_width stride width
 * @param vir_height stride height
 * @returns 0 ok, -1 error
 */
int input_single(int venc_channel_id, int video_fd, unsigned int width, unsigned int height, unsigned int vir_width, unsigned int vir_height)
{
    struct v4l2_buffer buf;
    struct v4l2_plane plane;

    memset(&buf, 0, sizeof(buf));
    memset(&plane, 0, sizeof(plane));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.length = 1;
    buf.m.planes = &plane;

    int ret = ioctl(video_fd, VIDIOC_DQBUF, &buf);
    if (ret == -1)
    {
        perror("video device dequeue buffer error");
        return -1;
    }
    printf("video device dequeue buffer %d %d %d ok\n", buf.index, buf.sequence, plane.bytesused);

    MB_BLK block = RK_MPI_MMZ_Fd2Handle(plane.m.fd);
    // check block
    if (block == RK_NULL)
    {
        errno = -1;
        perror("null block");
        return -1;
    }

    // there is no need to flush cache, but i do not known why
    // in `test_mpi_venc.cpp`, they have this step
    // int32_t ret = RK_MPI_SYS_MmzFlushCache(block, RK_FALSE);
    // if (ret != RK_SUCCESS)
    // {
    //     errno = ret;
    //     perror("mpi memory flush cache error");
    //     continue;
    // }
    // printf("mpi memory flush cache ok\n");

    VIDEO_FRAME_INFO_S frame;
    memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    frame.stVFrame.pMbBlk = block;
    frame.stVFrame.u32Width = width;
    frame.stVFrame.u32Height = height;
    frame.stVFrame.u32VirWidth = vir_width;
    frame.stVFrame.u32VirHeight = vir_height;
    frame.stVFrame.enPixelFormat = RK_COLOR;

    frame.stVFrame.u32TimeRef = buf.sequence;
    frame.stVFrame.u64PTS = time_to_us(buf.timestamp);

    frame.stVFrame.u32FrameFlag |= keep_running ? 0 : FRAME_FLAG_SNAP_END;

    // send frame
    ret = RK_MPI_VENC_SendFrame(venc_channel_id, &frame, TIMEOUT);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc send frame error");
        return -1;
    }
    printf("mpi venc send frame ok\n");

    ret = ioctl(video_fd, VIDIOC_QBUF, &buf);
    if (ret == -1)
    {
        perror("video device requeue buffer error");
        return -1;
    }
    printf("video device requeue buffer %d ok\n", buf.index);

    return 0;
}

/**
 * output single
 * @param venc_channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param stream stream pointer
 * @returns 0 ok, -1 error
 */
int output_single(int venc_channel_id, VENC_STREAM_S *stream)
{
    int ret = RK_MPI_VENC_GetStream(venc_channel_id, stream, TIMEOUT);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc get stream error");
        return -1;
    }
    printf("mpi venc get stream ok %d %d\n", stream->u32Seq, stream->pstPack->u64PTS);

    unsigned int size = stream->pstPack->u32Len;
    // read to destnation
    void *dst = RK_MPI_MB_Handle2VirAddr(stream->pstPack->pMbBlk);

    // save
    save_data(dst, size);

    stop_running();

    // release stream
    ret = RK_MPI_VENC_ReleaseStream(venc_channel_id, stream);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc release stream error");
        return -1;
    }
    printf("mpi venc release stream ok\n");

    // check end
    if (stream->pstPack->bStreamEnd == RK_TRUE)
    {
        return 1;
    }

    return 0;
}

typedef struct
{
    int venc_channel_id;
    int video_fd;
    unsigned int width;
    unsigned int height;
    unsigned int vir_width;
    unsigned int vir_height;
} input_args_t;

void *input(void *arg)
{
    input_args_t *args = (input_args_t *)arg;

    while (keep_running)
    {
        int ret = input_single(args->venc_channel_id, args->video_fd, args->width, args->height, args->vir_width, args->vir_height);
        if (ret == -1)
        {
            break;
        }
    }

    stop_running();
    return NULL;
}

typedef struct
{
    int venc_channel_id;
} output_args_t;

void *output(void *arg)
{
    output_args_t *args = (output_args_t *)arg;

    VENC_STREAM_S stream;
    stream.pstPack = malloc(sizeof(VENC_PACK_S));

    while (keep_running)
    {
        int ret = output_single(args->venc_channel_id, &stream);
        if (ret == 1)
        {
            // end break
            break;
        }
        else if (ret == -1)
        {
            // error break
            break;
        }
    }

    if (stream.pstPack)
    {
        free(stream.pstPack);
    }

    stop_running();
    return NULL;
}

int main()
{
    // regist signal
    signal(SIGINT, stop_running);
    signal(SIGTERM, stop_running);

    // calculate size
    MB_PIC_CAL_S cal;
    // 1920 1080 will be strided to 1920 1088
    // but v4l2 dma can not fill all byes, then venc think the frame is not end and will not work at all
    // that is why we should calculate manually
    // calculate_venc(VIDEO_WIDTH, VIDEO_HEIGHT, &cal);
    cal.u32VirWidth = VIDEO_WIDTH;
    cal.u32VirHeight = VIDEO_HEIGHT;
    cal.u32MBSize = VIDEO_WIDTH * VIDEO_HEIGHT * 3 / 2;
    printf("manual calculate ok %d %d %d\n", cal.u32VirWidth, cal.u32VirHeight, cal.u32MBSize);

    // init venc
    // i dont know why stream output buffer count is 8
    // in `test_mpi_venc.cpp`, they use 8
    int ret = init_venc(VENC_CHANNEL, VIDEO_WIDTH, VIDEO_HEIGHT, BIT_RATE, GOP, 8, cal);
    if (ret == 1)
    {
        return ret;
    }

    // init v4l2
    int video_fd = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    unsigned int buffer_count = init_v4l2_buffers(video_fd, BUFFER_COUNT);
    if (buffer_count == 0)
    {
        goto destroy_venc;
    }

    // init buffers
    unsigned int memory_pool = init_venc_memory(buffer_count, cal);
    if (memory_pool == MB_INVALID_POOLID)
    {
        goto destroy_v4l2;
    }

    void *blocks[BUFFER_COUNT];
    ret = allocate_buffers(memory_pool, video_fd, buffer_count, blocks);
    if (ret == -1)
    {
        goto destroy_v4l2;
    }

    // start venc
    ret = start_venc(VENC_CHANNEL);
    if (ret == -1)
    {
        goto free_buffers;
    }

    // start v4l2
    ret = start_v4l2(video_fd);
    if (ret == -1)
    {
        goto stop_venc;
    }

    // threads
    pthread_t input_thread;
    pthread_t output_thread;

    input_args_t input_args = {
        .venc_channel_id = VENC_CHANNEL,
        .video_fd = video_fd,
        .width = VIDEO_WIDTH,
        .height = VIDEO_HEIGHT,
        .vir_width = cal.u32VirWidth,
        .vir_height = cal.u32VirHeight,

    };
    output_args_t outpu_args = {
        .venc_channel_id = VENC_CHANNEL,
    };

    pthread_create(&input_thread, NULL, input, &input_args);
    pthread_create(&output_thread, NULL, output, &outpu_args);

    // wait thread end
    pthread_join(input_thread, NULL);
    pthread_join(output_thread, NULL);

    // stop v4l2
    stop_v4l2(video_fd);

stop_venc:
    // stop venc
    ret = stop_venc(VENC_CHANNEL);

free_buffers:
    // free buffer
    ret = free_buffers(BUFFER_COUNT, blocks);

destroy_v4l2:
    // destroy v4l2
    destroy_v4l2(video_fd);

destroy_venc:
    // destroy venc
    destroy_venc(VENC_CHANNEL, memory_pool);

    return ret;
}
