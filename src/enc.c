#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rk_common.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_type.h"

#define VENC_CHANNEL 0

static volatile uint32_t memory_pool = MB_INVALID_POOLID;
static volatile uint32_t block_size;
static volatile MB_BLK block = RK_NULL;
static volatile VENC_STREAM_S stream;

int init_venc(unsigned int bit_rate, unsigned int gop, unsigned int width, unsigned int height)
{
    int32_t ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi init error");
        return -1;
    }

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
    PIC_BUF_ATTR_S pic_buf_attr;
    MB_PIC_CAL_S mb_pic_cal;
    memset(&pic_buf_attr, 0, sizeof(PIC_BUF_ATTR_S));
    memset(&mb_pic_cal, 0, sizeof(MB_PIC_CAL_S));
    pic_buf_attr.u32Width = width;
    pic_buf_attr.u32Height = height;
    pic_buf_attr.enPixelFormat = RK_VIDEO_FMT_YUV;
    ret = RK_MPI_CAL_COMM_GetPicBufferSize(&pic_buf_attr, &mb_pic_cal);
    venc_attr.stVencAttr.u32BufSize = mb_pic_cal.u32MBSize;
    block_size = mb_pic_cal.u32MBSize;

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = bit_rate;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = gop;

    ret = RK_MPI_VENC_CreateChn(VENC_CHANNEL, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel create error");
        goto error1;
    }

    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = mb_pic_cal.u32MBSize;
    memory_pool_config.u32MBCnt = 1;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    memory_pool = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool == MB_INVALID_POOLID)
    {
        errno = -1;
        perror("mpi memory pool create error");
        goto error2;
    }

    return 0;

error2:
    RK_MPI_VENC_DestroyChn(VENC_CHANNEL);
    goto error1;
error1:
    RK_MPI_SYS_Exit();
    return -1;
}

int start_venc()
{
    // start recive frame
    VENC_RECV_PIC_PARAM_S receive_param;
    memset(&receive_param, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    receive_param.s32RecvPicNum = -1;
    int32_t ret = RK_MPI_VENC_StartRecvFrame(VENC_CHANNEL, &receive_param);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi start receive frame error");
        return -1;
    }

    stream.pstPack = malloc(sizeof(VENC_PACK_S));

    return 0;
}

void *use_venc_frame()
{
    // release exist block
    if (block != RK_NULL)
    {
        RK_MPI_MB_ReleaseMB(block);
    }

    // get block
    block = RK_MPI_MB_GetMB(memory_pool, block_size, RK_TRUE);
    if (block == RK_NULL)
    {
        errno = -1;
        perror("mpi memory get block error");
        return NULL;
    }

    void *vir_addr = RK_MPI_MB_Handle2VirAddr(block);

    return vir_addr;
}

int send_venc_frame(unsigned int width, unsigned int height, bool is_end)
{
    // check block
    if (block == RK_NULL)
    {
        errno = -1;
        perror("no valid block available, call use_venc_frame first");
        return -1;
    }

    int32_t ret = RK_MPI_SYS_MmzFlushCache(block, RK_FALSE);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi memory flush cache error");
        goto error;
    }

    VIDEO_FRAME_INFO_S frame;
    memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    frame.stVFrame.pMbBlk = block;
    frame.stVFrame.u32Width = width;
    frame.stVFrame.u32Height = height;
    frame.stVFrame.u32VirWidth = width;
    frame.stVFrame.u32VirHeight = height;
    frame.stVFrame.enPixelFormat = RK_VIDEO_FMT_YUV;
    frame.stVFrame.u32FrameFlag = is_end ? FRAME_FLAG_SNAP_END : 0;

    // send frame
    ret = RK_MPI_VENC_SendFrame(VENC_CHANNEL, &frame, -1);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc send frame error");
        goto error;
    }

    // release block
    ret = RK_MPI_MB_ReleaseMB(block);
    block = RK_NULL;
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi memory release block error");
        return -1;
    }

    return 0;

error:
    if (block != RK_NULL)
    {
        ret = RK_MPI_MB_ReleaseMB(block);
        block = RK_NULL;
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
        }
    }

    return -1;
}

int read_venc_frame(void **dst, unsigned int *size)
{
    // get stream
    int32_t ret = RK_MPI_VENC_GetStream(VENC_CHANNEL, &stream, -1);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc get stream error");
        return -1;
    }

    // read to destnation
    void *d = RK_MPI_MB_Handle2VirAddr(stream.pstPack->pMbBlk);
    *dst = d;
    *size = stream.pstPack->u32Len;

    return 0;
}

int release_venc_frame()
{
    // release stream
    int32_t ret = RK_MPI_VENC_ReleaseStream(VENC_CHANNEL, &stream);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc release stream error");
        return -1;
    }

    if (stream.pstPack->bStreamEnd == RK_TRUE)
    {
        return 1;
    }

    return 0;
}

void stop_venc()
{
    int32_t ret = 0;

    // free stream
    if (stream.pstPack)
    {
        free(stream.pstPack);
        stream.pstPack = NULL;
    }

    // release block
    if (block != RK_NULL)
    {
        ret = RK_MPI_MB_ReleaseMB(block);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
        }
        block = RK_NULL;
    }

    // stop
    ret = RK_MPI_VENC_StopRecvFrame(VENC_CHANNEL);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi stop receive frame error");
    }

    if (memory_pool != MB_INVALID_POOLID)
    {
        ret = RK_MPI_MB_DestroyPool(memory_pool);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory pool destory error");
        }
    }
}

void close_venc()
{
    int32_t ret = RK_MPI_VENC_DestroyChn(VENC_CHANNEL);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel destory error");
    }

    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi exit error");
    }
}
