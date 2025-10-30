#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <unistd.h>
#include <fcntl.h>

#include "rk_common.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_type.h"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080

#define VENC_CHANNEL 0
#define BIT_RATE 10 * 1024
#define GOP 2

#define VIDEO_PATH "/dev/video0"
#define BUFFER_COUNT 4

/**
 * mpi memory pool
 */
static uint32_t memory_pool = MB_INVALID_POOLID;

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

    // calculate size
    venc_attr.stVencAttr.u32BufSize = size;
    printf("mpi calculate ok\n");

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = bit_rate;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = gop;

    ret = RK_MPI_VENC_CreateChn(VENC_CHANNEL, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel create error");
        return -1;
    }
    printf("mpi venc channel create ok\n");

    return 0;
}

int init_venc_memory(unsigned int buffer_count, unsigned long long int size)
{
    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = size;
    memory_pool_config.u32MBCnt = buffer_count;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    memory_pool = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool == MB_INVALID_POOLID)
    {
        errno = -1;
        perror("mpi memory pool create error");
        return 0;
    }
    printf("mpi memory pool create ok\n");

    return memory_pool;
}

void close_venc(int channel_id)
{
    int32_t ret = RK_MPI_VENC_DestroyChn(channel_id);
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

int init_v4l2(const char *path, int width, int height)
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

unsigned int init_v4l2_buffers(int fd, unsigned int buffer_count)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("video device request buffer error");
        close(fd);
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video buffers not support dma");
        close(fd);
        return 0;
    }
    printf("video buffers support dma\n");

    if (req.count < 2)
    {
        perror("video buffers count not enough");
        close(fd);
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

struct buffer
{
    MB_BLK block;
    struct v4l2_buffer vbuffer;
};

int init_buffers(int fd, unsigned int buffer_count, struct buffer buffers[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // use v4l2 buffer
        struct v4l2_plane vplane;

        memset(&buffers[i].vbuffer, 0, sizeof(buffers[i].vbuffer));
        memset(&vplane, 0, sizeof(vplane));
        buffers[i].vbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buffers[i].vbuffer.memory = V4L2_MEMORY_DMABUF;
        buffers[i].vbuffer.index = i;
        buffers[i].vbuffer.length = 1;
        buffers[i].vbuffer.m.planes = &vplane;

        // although we use dma, query buffer is also necessary
        // this will init vplane
        if (ioctl(fd, VIDIOC_QUERYBUF, &buffers[i].vbuffer) == -1)
        {
            perror("video device query buffer error");
            close(fd);
            return -1;
        }
        printf("video device query buffer %d ok\n", i);

        // get mpi block
        MB_BLK block = RK_MPI_MB_GetMB(memory_pool, vplane.length, RK_TRUE);
        if (block == RK_NULL)
        {
            errno = -1;
            perror("mpi memory get block error");
            return -1;
        }
        printf("mpi memory get block %d ok\n", i);
        buffers[i].block = block;

        int block_fd = RK_MPI_MMZ_Handle2Fd(block);
        if (block_fd == -1)
        {
            errno = -1;
            perror("mpi memory get block fd error");
            return -1;
        }
        printf("mpi memory get block %d ok\n", i);

        // set block fd to plane
        vplane.m.fd = block_fd;

        if (ioctl(fd, VIDIOC_QBUF, &buffers[i].vbuffer) == -1)
        {
            perror("video device queue buffer error");
            return -1;
        }
        printf("video device queue buffer %d ok\n", i);
    }

    return 0;
}

int release_buffers(unsigned int buffer_count, struct buffer buffers[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        int ret = RK_MPI_MB_ReleaseMB(buffers[i].block);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
            return -1;
        }
        printf("mpi memory release block ok\n");
    }

    return 0;
}

int main()
{
    // init venc
    unsigned int size = calculate_mpi_size(VIDEO_WIDTH, VIDEO_HEIGHT);
    init_venc(VENC_CHANNEL, VIDEO_WIDTH, VIDEO_HEIGHT, BIT_RATE, GOP, size);

    // init v4l2
    int video_fd = init_v4l2(VIDEO_PATH, VIDEO_WIDTH, VIDEO_HEIGHT);
    unsigned int buffer_count = init_v4l2_buffers(video_fd, BUFFER_COUNT);

    // init buffers
    int memory_pool = init_venc_memory(buffer_count, size);
    struct buffer buffers[BUFFER_COUNT];
    init_buffers(video_fd, buffer_count, buffers);

    // start venc
    start_venc(VENC_CHANNEL);

    // start v4l2
    start_v4l2(video_fd);

    // loop here

    // stop v4l2
    stop_v4l2(video_fd);

    // stop venc
    stop_venc(VENC_CHANNEL);

    // release buffer
    release_buffers(BUFFER_COUNT, buffers);

    // close v4l2
    close_v4l2(video_fd);
    // close venc
    close_venc(VENC_CHANNEL);

    return 0;
}
