#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include "rk_common.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_type.h"

#include "utils.h"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080

#define VENC_CHANNEL 0
#define BIT_RATE 10 * 1024
#define GOP 2
#define TIMEOUT 1000

#define VIDEO_PATH "/dev/video0"
#define BUFFER_COUNT 4

#define OUPUT_PATH "data/frame.h264"

int save_data(void *data, unsigned int size)
{
    FILE *f = fopen(OUPUT_PATH, "wb");
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

/**
 * running
 */
static volatile bool keep_running = true;

unsigned int calculate_mpi_size(unsigned int width, unsigned int height)
{
    PIC_BUF_ATTR_S pic_buf_attr;
    MB_PIC_CAL_S mb_pic_cal;
    memset(&pic_buf_attr, 0, sizeof(PIC_BUF_ATTR_S));
    memset(&mb_pic_cal, 0, sizeof(MB_PIC_CAL_S));
    pic_buf_attr.u32Width = width;
    pic_buf_attr.u32Height = height;
    pic_buf_attr.enPixelFormat = RK_FMT_YUV422_YUYV;

    int32_t ret = RK_MPI_CAL_COMM_GetPicBufferSize(&pic_buf_attr, &mb_pic_cal);
    if (ret == -1)
    {
        errno = ret;
        perror("mpi calcualte error");
        return 0;
    }

    return mb_pic_cal.u32MBSize;
}

int init_venc(int channel_id, unsigned int width, unsigned int height, unsigned int bit_rate, unsigned int gop, unsigned int size)
{
    int32_t ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi init error");
        return -1;
    }
    printf("mpi init ok\n");

    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));

    // set h264
    venc_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;
    venc_attr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    venc_attr.stVencAttr.u32PicWidth = width;
    venc_attr.stVencAttr.u32PicHeight = height;
    venc_attr.stVencAttr.u32VirWidth = width;
    venc_attr.stVencAttr.u32VirHeight = height;
    venc_attr.stVencAttr.u32StreamBufCnt = 8;
    venc_attr.stVencAttr.u32BufSize = size;

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = bit_rate;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = gop;

    ret = RK_MPI_VENC_CreateChn(channel_id, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel create error");
        return -1;
    }
    printf("mpi venc channel create ok\n");

    return 0;
}

unsigned int init_venc_memory(unsigned int buffer_count, unsigned long long int size)
{
    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = size;
    memory_pool_config.u32MBCnt = buffer_count;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    unsigned int memory_pool = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool == MB_INVALID_POOLID)
    {
        errno = -1;
        perror("mpi memory pool create error");
        return MB_INVALID_POOLID;
    }
    printf("mpi memory pool create ok\n");

    return memory_pool;
}

void close_venc(int channel_id, unsigned int memory_pool)
{
    int ret = RK_MPI_MB_DestroyPool(memory_pool);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi memory pool destory error");
    }

    ret = RK_MPI_VENC_DestroyChn(channel_id);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel destory error");
    }
    printf("mpi venc channel destory ok\n");

    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi exit error");
    }
    printf("mpi exit ok\n");
}

int start_venc(int channel_id)
{
    VENC_RECV_PIC_PARAM_S receive_param;
    memset(&receive_param, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    receive_param.s32RecvPicNum = -1;
    int ret = RK_MPI_VENC_StartRecvFrame(channel_id, &receive_param);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi start receive frame error");
        return -1;
    }
    printf("mpi start receive frame ok\n");

    return 0;
}

int stop_venc(int channel_id)
{
    int ret = RK_MPI_VENC_StopRecvFrame(channel_id);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi stop receive frame error");
        return -1;
    }
    printf("mpi stop receive frame ok\n");

    return 0;
}

int init_v4l2(const char *path, unsigned int width, unsigned int height)
{
    struct v4l2_capability cap;

    int fd = open(path, O_RDWR);
    if (fd == -1)
    {
        perror("video device open error");
        return 0;
    }
    printf("video device open ok\n");

    // check device capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        perror("video device query capabilities error");
        close(fd);
        return 0;
    }
    printf("video device query capabilities ok\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        perror("video device not support capture multiple plane");
        close(fd);
        return 0;
    }
    printf("video device support capture multiple plane\n");

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        perror("video device not support streaming");
        close(fd);
        return 0;
    }
    printf("video device support streaming\n");

    // setup start
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    // fmt.fmt.pix_mp.num_planes = 1;

    // fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    // fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height;
    // fmt.fmt.pix_mp.plane_fmt[1].bytesperline = width;
    // fmt.fmt.pix_mp.plane_fmt[1].sizeimage = width * height / 2;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("video device set format error");
        close(fd);
        return 0;
    }
    printf("video device set format ok\n");
    // setup end

    return fd;
}

void close_v4l2(int fd)
{
    close(fd);
    printf("video device close ok\n");
}

unsigned int init_v4l2_buffers(int video_fd, unsigned int buffer_count)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video buffers not support dma");
        return 0;
    }
    printf("video buffers support dma\n");

    if (req.count < 2)
    {
        perror("video buffers count not enough");
        return 0;
    }
    printf("video buffers count %d\n", req.count);

    return req.count;
}

int start_v4l2(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        perror("video device start streaming error");
        return -1;
    }
    printf("video device start streaming ok\n");

    return 0;
}

int stop_v4l2(int fd)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
    {
        perror("video device stop stream error");
        return -1;
    }
    printf("video device stop stream ok\n");

    return 0;
}

int init_buffers(int video_fd, unsigned int memory_pool, unsigned int buffer_count)
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // use v4l2 buffer
        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.index = i;
        buf.length = 1;
        buf.m.planes = &plane;

        // although we use dma, query buffer is also necessary, this will init plane
        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            perror("video device query buffer error");
            return -1;
        }
        printf("video device query buffer %d ok\n", i);

        // get mpi block
        MB_BLK block = RK_MPI_MB_GetMB(memory_pool, plane.length, RK_TRUE);
        if (block == RK_NULL)
        {
            errno = -1;
            perror("mpi memory get block error");
            return -1;
        }
        printf("mpi memory get block %d ok\n", i);

        int block_fd = RK_MPI_MB_Handle2Fd(block);
        if (block_fd == -1)
        {
            errno = -1;
            perror("mpi memory get block fd error");
            return -1;
        }
        printf("mpi memory get block %d ok\n", i);

        // set block fd to plane
        plane.m.fd = block_fd;

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) == -1)
        {
            perror("video device queue buffer error");
            return -1;
        }
        printf("video device queue buffer %d ok\n", i);
    }

    return 0;
}

int release_buffers(int video_fd, unsigned int buffer_count)
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.length = 1;
        buf.m.planes = &plane;

        if (ioctl(video_fd, VIDIOC_DQBUF, &buf) == -1)
        {
            perror("video device dequeue buffer error");
            continue;
        }

        MB_BLK block = RK_MPI_MMZ_Fd2Handle(plane.m.fd);
        if (block == NULL)
        {
            errno = -1;
            perror("null block");
            continue;
        }

        int ret = RK_MPI_MB_ReleaseMB(block);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
            continue;
        }
        printf("mpi memory release block ok\n");
    }

    return 0;
}

typedef struct
{
    int video_fd;
    unsigned int width;
    unsigned int height;

    int venc_channel_id;
} input_args_t;

void *input(void *arg)
{
    input_args_t *args = (input_args_t *)arg;

    unsigned int frame_num = 0;

    while (keep_running)
    {
        keep_running = false;

        struct v4l2_buffer buf;
        struct v4l2_plane plane;

        memset(&buf, 0, sizeof(buf));
        memset(&plane, 0, sizeof(plane));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.length = 1;
        buf.m.planes = &plane;

        if (ioctl(args->video_fd, VIDIOC_DQBUF, &buf) == -1)
        {
            perror("video device dequeue buffer error");
            continue;
        }
        printf("video device dequeue buffer %d %d ok\n", buf.index, buf.sequence);

        MB_BLK block = RK_MPI_MMZ_Fd2Handle(plane.m.fd);
        // check block
        if (block == RK_NULL)
        {
            errno = -1;
            perror("null block");
            continue;
        }

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
        frame.stVFrame.u32Width = args->width;
        frame.stVFrame.u32Height = args->height;
        frame.stVFrame.u32VirWidth = args->width;
        frame.stVFrame.u32VirHeight = args->height;
        frame.stVFrame.enPixelFormat = RK_FMT_YUV422_YUYV;

        frame.stVFrame.u32TimeRef = buf.sequence;
        frame.stVFrame.u64PTS = time_to_us(buf.timestamp);

        frame.stVFrame.u32FrameFlag |= keep_running ? 0 : FRAME_FLAG_SNAP_END;

        // send frame
        int32_t ret = RK_MPI_VENC_SendFrame(args->venc_channel_id, &frame, TIMEOUT);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi venc send frame error");
            continue;
        }
        printf("mpi venc send frame ok\n");

        frame_num += 1;
    }

    return NULL;
}

typedef struct
{
    int venc_channel_id;
} output_args_t;

void *output(void *arg)
{
    output_args_t *args = (output_args_t *)arg;

    while (keep_running)
    {
        // get stream
        VENC_STREAM_S stream;
        stream.pstPack = malloc(sizeof(VENC_PACK_S));

        int ret = RK_MPI_VENC_GetStream(args->venc_channel_id, &stream, 1000);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi venc get stream error");
            continue;
        }
        printf("mpi venc get stream ok %d %d\n", stream.u32Seq, stream.pstPack->u64PTS);

        void *dst;
        unsigned int size = stream.pstPack->u32Len;
        // read to destnation
        dst = RK_MPI_MB_Handle2VirAddr(stream.pstPack->pMbBlk);

        // save
        save_data(dst, size);

        // release stream
        ret = RK_MPI_VENC_ReleaseStream(VENC_CHANNEL, &stream);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi venc release stream error");
            continue;
        }
        printf("mpi venc release stream ok\n");

        // check end
        if (stream.pstPack->bStreamEnd == RK_TRUE)
        {
            break;
        }
    }

    return NULL;
}

void main_stop()
{
    keep_running = true;
}

int main()
{
    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    // init venc
    unsigned int size = calculate_mpi_size(VIDEO_WIDTH, VIDEO_HEIGHT);
    init_venc(VENC_CHANNEL, VIDEO_WIDTH, VIDEO_HEIGHT, BIT_RATE, GOP, size);

    // init v4l2
    int video_fd = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    unsigned int buffer_count = init_v4l2_buffers(video_fd, BUFFER_COUNT);

    // init buffers
    unsigned int memory_pool = init_venc_memory(buffer_count, size);
    init_buffers(video_fd, memory_pool, buffer_count);

    // start venc
    start_venc(VENC_CHANNEL);

    // start v4l2
    start_v4l2(video_fd);

    // threads
    // pthread_t input_thread;
    // pthread_t output_thread;

    input_args_t input_args = {
        .video_fd = video_fd,
        .width = VIDEO_WIDTH,
        .height = VIDEO_HEIGHT,

        .venc_channel_id = VENC_CHANNEL,
    };
    output_args_t outpu_args = {
        .venc_channel_id = VENC_CHANNEL,
    };

    // pthread_create(&input_thread, NULL, input, &input_args);
    // pthread_create(&output_thread, NULL, output, &outpu_args);

    // // wait thread end
    // pthread_join(input_thread, NULL);
    // pthread_join(output_thread, NULL);

    // stop v4l2
    stop_v4l2(video_fd);

    // stop venc
    stop_venc(VENC_CHANNEL);

    // release buffer
    release_buffers(video_fd, BUFFER_COUNT);

    // close v4l2
    close_v4l2(video_fd);
    // close venc
    close_venc(VENC_CHANNEL, memory_pool);

    return 0;
}
