#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "rk_mpi_sys.h"
#include "rk_common.h"
#include "rk_type.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"

#include "buffer.h"

#define SUCCESS 0
#define ERROR -1

#define VENC_CHANNEL 0

MB_POOL memory_pool = MB_INVALID_POOLID;

// https://wiki.luckfox.com/zh/Luckfox-Pico-Pro-Max/MPI/

int error(int32_t ret, const char *__restrict__ __format)
{
    fprintf(stderr, __format, ret);
    return ERROR;
}

int init_venc(unsigned int bit_rate, unsigned int gop, unsigned int width, unsigned int height, unsigned int buffer_count)
{
    int32_t ret = 0;

    ret = RK_MPI_SYS_Init();

    if (ret != RK_SUCCESS)
    {
        return error(ret, "mpi init error %x\n");
    }

    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));

    // set h264
    venc_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;
    venc_attr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    venc_attr.stVencAttr.u32PicWidth = width;
    venc_attr.stVencAttr.u32PicHeight = height;
    venc_attr.stVencAttr.u32StreamBufCnt = buffer_count;
    venc_attr.stVencAttr.u32BufSize = width * height * 3 / 2;

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = bit_rate;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = gop;

    ret = RK_MPI_VENC_CreateChn(VENC_CHANNEL, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel create error %x\n");
    }

    return SUCCESS;
}

int start_venc(unsigned int width, unsigned int height)
{
    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(memory_pool_config));
    memory_pool_config.u64MBSize = width * height * BUFFER_COUNT;
    memory_pool_config.u32MBCnt = BUFFER_COUNT;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;
    memory_pool = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool == MB_INVALID_POOLID)
    {
        return error(RK_FAILURE, "memory pool create error");
    }

    int32_t ret = 0;

    VENC_RECV_PIC_PARAM_S param;
    memset(&param, 0, sizeof(param));

    param.s32RecvPicNum = BUFFER_COUNT;

    ret = RK_MPI_VENC_StartRecvFrame(VENC_CHANNEL, &param);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel start error %x\n");
    }

    return SUCCESS;
}

int use_venc_data()
{
    for (unsigned int i = 0; i < BUFFER_COUNT; i++)
    {
        // MB_BLK block = RK_MPI_MB_GetMB(memory_pool, plane_size);
    }

    VENC_STREAM_S stream;
    stream.pstPack = malloc(sizeof(VENC_PACK_S));

    int32_t ret = 0;

    // time out 100ms
    ret = RK_MPI_VENC_GetStream(VENC_CHANNEL, &stream, 100);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel get stream error %x\n");
    }

    // data = RK_MPI_MB_Handle2VirAddr(stream.pstPack->pMbBlk);

    ret = RK_MPI_VENC_ReleaseStream(VENC_CHANNEL, &stream);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel release stream error %x\n");
    }

    return SUCCESS;
}

int stop_venc()
{
    int32_t ret = 0;

    ret = RK_MPI_VENC_StopRecvFrame(VENC_CHANNEL);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel stop error %x\n");
    }

    if (memory_pool != MB_INVALID_POOLID)
    {
        ret = RK_MPI_MB_DestroyPool(memory_pool);
        if (ret != RK_SUCCESS)
        {
            return error(ret, "memory pool destory error %x");
        }
    }

    return SUCCESS;
}

int close_venc()
{
    int32_t ret = 0;

    ret = RK_MPI_VENC_DestroyChn(VENC_CHANNEL);

    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel close error %x\n");
    }

    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS)
    {
        return error(ret, "mpi exit error %x\n");
    }

    return SUCCESS;
}
