#include "video.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <fcntl.h>
#include <unistd.h>

#include "rk_common.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_type.h"

#include "utils.h"

#define PLANES 2

int calculate_venc(uint32_t width, uint32_t height, MB_PIC_CAL_S *mb_pic_cal)
{

    PIC_BUF_ATTR_S pic_buf_attr;
    memset(&pic_buf_attr, 0, sizeof(PIC_BUF_ATTR_S));
    memset(mb_pic_cal, 0, sizeof(MB_PIC_CAL_S));
    pic_buf_attr.u32Width = width;
    pic_buf_attr.u32Height = height;
    pic_buf_attr.enPixelFormat = RK_FMT_YUV420SP;

    int32_t ret = RK_MPI_CAL_COMM_GetPicBufferSize(&pic_buf_attr, mb_pic_cal);
    if (ret == -1)
    {
        errno = ret;
        perror("mpi calcualte error");
        return -1;
    }
    printf("mpi calculate ok %d %d %d\n", mb_pic_cal->u32VirWidth, mb_pic_cal->u32VirHeight, mb_pic_cal->u32MBSize);

    return 0;
}

int init_venc(int32_t channel_id, uint32_t width, uint32_t height, uint32_t bit_rate, uint32_t gop, uint32_t buffer_count, MB_PIC_CAL_S mb_pic_cal)
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
    venc_attr.stVencAttr.enType = RK_VIDEO_ID_AVC; // 8 h264 9 mjpeg 12 h265 15 jpeg
    venc_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
    venc_attr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    venc_attr.stVencAttr.u32PicWidth = width;
    venc_attr.stVencAttr.u32PicHeight = height;
    venc_attr.stVencAttr.u32VirWidth = width;
    venc_attr.stVencAttr.u32VirHeight = height;
    venc_attr.stVencAttr.u32StreamBufCnt = buffer_count;
    venc_attr.stVencAttr.u32BufSize = calculate_pic_byte_size(width, height, V4L2_PIX_FMT_NV12);

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = bit_rate;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = gop;

    ret = RK_MPI_VENC_CreateChn(channel_id, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc create channel error");
        RK_MPI_SYS_Exit();
        return -1;
    }
    printf("mpi venc %d create channel ok\n", channel_id);

    return 0;
}

uint32_t init_venc_memory(uint32_t buffer_count, MB_PIC_CAL_S mb_pic_cal)
{
    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = mb_pic_cal.u32MBSize;
    memory_pool_config.u32MBCnt = buffer_count;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    uint32_t memory_pool_id = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool_id == MB_INVALID_POOLID)
    {
        errno = -1;
        perror("mpi memory pool create error");
        return MB_INVALID_POOLID;
    }
    printf("mpi memory pool %d %d create ok\n", memory_pool_id, buffer_count);

    return memory_pool_id;
}

void destroy_venc(int32_t channel_id, uint32_t memory_pool_id)
{
    int32_t ret = RK_SUCCESS;

    // avoid invalid pool
    if (memory_pool_id != MB_INVALID_POOLID)
    {
        ret = RK_MPI_MB_DestroyPool(memory_pool_id);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory pool destory error");
        }
        printf("mpi memory pool %d destory ok\n", memory_pool_id);
    }

    ret = RK_MPI_VENC_DestroyChn(channel_id);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc destory error");
    }
    printf("mpi venc %d destory ok\n", channel_id);

    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi exit error");
    }
    printf("mpi exit ok\n");
}

int start_venc(int32_t channel_id)
{
    VENC_RECV_PIC_PARAM_S receive_param;
    memset(&receive_param, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    receive_param.s32RecvPicNum = -1;
    int32_t ret = RK_MPI_VENC_StartRecvFrame(channel_id, &receive_param);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc start receive frame error");
        return -1;
    }
    printf("mpi venc %d start receive frame ok\n", channel_id);

    return 0;
}

int stop_venc(int32_t channel_id)
{
    int32_t ret = RK_MPI_VENC_StopRecvFrame(channel_id);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc stop receive frame error");
        return -1;
    }
    printf("mpi venc %d stop receive frame ok\n", channel_id);

    return 0;
}

int init_v4l2(const char *path, unsigned int width, unsigned int height)
{
    struct v4l2_capability cap;

    int fd = open(path, O_RDWR);
    if (fd == -1)
    {
        perror("video device open error");
        return -1;
    }
    printf("video device open ok\n");

    // check device capabilities
    int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret == -1)
    {
        perror("video device query capabilities error");
        close(fd);
        return -1;
    }
    printf("video device query capabilities ok\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        perror("video device not support capture multiple plane");
        close(fd);
        return -1;
    }
    printf("video device support capture multiple plane\n");

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        perror("video device not support streaming");
        close(fd);
        return -1;
    }
    printf("video device support streaming\n");

    // set format
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    // fmt.fmt.pix_mp.num_planes = 1;

    // fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    // fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 3 / 2;

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret == -1)
    {
        perror("video device set format error");
        close(fd);
        return -1;
    }
    printf("video device set format ok\n");

    return fd;
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
        return 0;
    }
    printf("video device request buffer ok\n");

    if (!(req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF))
    {
        perror("video device not support dma");
        return 0;
    }
    printf("video device support dma\n");

    if (req.count < 2)
    {
        perror("video device buffers count not enough");
        return 0;
    }
    printf("video device buffers count %d\n", req.count);

    return req.count;
}

void destroy_v4l2(int fd)
{
    int ret = close(fd);
    if (ret == -1)
    {
        perror("video device close error");
    }
    printf("video device close ok\n");
}

int start_v4l2(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    int ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret == -1)
    {
        perror("video device start stream error");
        return -1;
    }
    printf("video device start stream ok\n");

    return 0;
}

int stop_v4l2(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    int ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret == -1)
    {
        perror("video device stop stream error");
        return -1;
    }
    printf("video device stop stream ok\n");

    return 0;
}

int allocate_buffers(uint32_t memory_pool_id, int video_fd, unsigned int buffer_count, void *blocks[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // use v4l2 buffer
        struct v4l2_buffer buf;
        struct v4l2_plane planes[PLANES];

        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.index = i;
        buf.length = 1;
        buf.m.planes = &planes;

        // although we use dma, query buffer is also necessary, this will init plane
        int ret = ioctl(video_fd, VIDIOC_QUERYBUF, &buf);
        if (ret == -1)
        {
            perror("video device query buffer error");
            return -1;
        }
        printf("video device query buffer %d ok %d %d\n", i, planes[0].length, planes[1].length);

        // get mpi block plane 0
        unsigned int bi = i * 2;
        blocks[bi] = RK_MPI_MB_GetMB(memory_pool_id, planes[0].length, RK_TRUE);
        if (blocks[bi] == RK_NULL)
        {
            errno = -1;
            perror("mpi memory get block error");
            return -1;
        }
        printf("mpi memory get block %d %dok\n", i, 0);

        // get block fd
        int32_t block_fd = RK_MPI_MB_Handle2Fd(blocks[bi]);
        if (block_fd == -1)
        {
            errno = -1;
            perror("mpi memory get block fd error");
            return -1;
        }
        printf("mpi memory get block %d %dok\n", i, 0);

        // set block fd to plane 0
        planes[0].m.fd = block_fd;

        // get mpi block plane 1
        bi += 1;
        blocks[bi] = RK_MPI_MB_GetMB(memory_pool_id, planes[1].length, RK_TRUE);
        if (blocks[bi] == RK_NULL)
        {
            errno = -1;
            perror("mpi memory get block error");
            return -1;
        }
        printf("mpi memory get block %d %dok\n", i, 1);

        // get block fd
        block_fd = RK_MPI_MB_Handle2Fd(blocks[bi]);
        if (block_fd == -1)
        {
            errno = -1;
            perror("mpi memory get block fd error");
            return -1;
        }
        printf("mpi memory get block %d %dok\n", i, 1);

        // set block fd to plane 1
        planes[1].m.fd = block_fd;

        ret = ioctl(video_fd, VIDIOC_QBUF, &buf);
        if (ret == -1)
        {
            perror("video device queue buffer error");
            return -1;
        }
        printf("video device queue buffer %d ok\n", i);
    }

    return 0;
}

int free_buffers(unsigned int buffer_count, void *blocks[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // release block plane 0
        unsigned int bi = i * 2;
        int32_t ret = RK_MPI_MB_ReleaseMB(blocks[bi]);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
            continue;
        }

        // release block plane 1
        bi += 1;
        ret = RK_MPI_MB_ReleaseMB(blocks[bi]);
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

int input(int32_t venc_channel_id, int video_fd, uint32_t width, uint32_t height, uint32_t vir_width, uint32_t vir_height, bool is_end, int32_t timeout)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[PLANES];

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.length = 1;
    buf.m.planes = &planes;

    int32_t ret = ioctl(video_fd, VIDIOC_DQBUF, &buf);
    if (ret == -1)
    {
        perror("video device dequeue buffer error");
        return -1;
    }
    // printf("video device dequeue buffer %d %d %d ok\n", buf.index, buf.sequence, plane.bytesused);

    // get block plane 0
    MB_BLK block = RK_MPI_MMZ_Fd2Handle(planes[0].m.fd);
    // check block
    if (block == RK_NULL)
    {
        errno = -1;
        perror("null block");
        return -1;
    }

    // get virture address plane 0
    void *va0 = RK_MPI_MB_Handle2VirAddr(block);

    // get block plane 1
    block = RK_MPI_MMZ_Fd2Handle(planes[1].m.fd);
    // check block
    if (block == RK_NULL)
    {
        errno = -1;
        perror("null block");
        return -1;
    }

    // get virture address plane 1
    void *va1 = RK_MPI_MB_Handle2VirAddr(block);

    // there is no need to flush cache, but i do not know why
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
    frame.stVFrame.u32Width = width;
    frame.stVFrame.u32Height = height;
    frame.stVFrame.u32VirWidth = width;
    frame.stVFrame.u32VirHeight = height;
    frame.stVFrame.enPixelFormat = RK_FMT_YUV420SP;
    frame.stVFrame.pVirAddr[RK_COLOR_YUV_Y_PLANE] = va0;
    frame.stVFrame.pVirAddr[RK_COLOR_YUV_UV_PLANE] = va1;

    frame.stVFrame.u32TimeRef = buf.sequence;
    frame.stVFrame.u64PTS = time_to_us(buf.timestamp);

    frame.stVFrame.u32FrameFlag |= is_end ? FRAME_FLAG_SNAP_END : 0;

    // send frame
    ret = RK_MPI_VENC_SendFrame(venc_channel_id, &frame, timeout);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc send frame error");
        return -1;
    }
    // printf("mpi venc send frame ok\n");

    ret = ioctl(video_fd, VIDIOC_QBUF, &buf);
    if (ret == -1)
    {
        perror("video device requeue buffer error");
        return -1;
    }
    // printf("video device requeue buffer %d ok\n", buf.index);

    return 0;
}

int output(int32_t venc_channel_id, VENC_STREAM_S *stream, int32_t timeout, output_data_callback callback, void *callback_context)
{
    int32_t ret = RK_MPI_VENC_GetStream(venc_channel_id, stream, timeout);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc get stream error");
        return -1;
    }
    // printf("mpi venc get stream ok %d %d\n", stream->u32Seq, stream->pstPack->u64PTS);

    uint32_t size = stream->pstPack->u32Len;
    // read to destnation
    void *dst = RK_MPI_MB_Handle2VirAddr(stream->pstPack->pMbBlk);

    int ret2 = callback(stream->u32Seq, stream->pstPack->u64PTS, dst, size, callback_context);

    // release stream
    ret = RK_MPI_VENC_ReleaseStream(venc_channel_id, stream);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc release stream error");
        return -1;
    }
    // printf("mpi venc release stream ok\n");

    return ret2;
}
