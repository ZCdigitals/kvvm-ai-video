#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_cal.h"

#define INPUT_PATH "/root/frame.raw"
#define OUPUT_PATH "/root/frame.h264"

#define FRAME_COUNT 1
#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080

#define VENC_CHANNEL 0

// read `media/rockit/rockit/mpi/example/mod/test_mpi_venc.cpp`

int success(const char *msg)
{
    fprintf(stdout, msg);
}

int error(int32_t ret, const char *__restrict__ __format)
{
    fprintf(stderr, __format, ret);
    return -1;
}

int32_t read_image_nv12(void *vir_addr, uint32_t width, uint32_t height, uint32_t vir_width, uint32_t vir_height, FILE *f)
{
    uint32_t row = 0;
    uint32_t size = 0;

    uint8_t *buffer_y = vir_addr;
    uint8_t *buffer_u = buffer_y + vir_width * vir_height;
    uint8_t *buffer_v = buffer_u + vir_width * vir_height / 4;

    for (row = 0; row < vir_height; row++)
    {
        size = fread(buffer_y + row * vir_width, 1, width, f);
        if (size != width)
        {
            return RK_FAILURE;
        }
    }

    for (row = 0; row < vir_height / 2; row++)
    {
        size = fread(buffer_u + row * vir_width, 1, width, f);
        if (size != width)
        {
            return RK_FAILURE;
        }
    }

    return RK_SUCCESS;
}

int main()
{
    int32_t ret = 0;

    ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS)
    {
        return error(ret, "mpi init error %x\n");
    }
    success("mpi init ok\n");

    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(VENC_CHN_ATTR_S));

    // set h264
    venc_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;
    venc_attr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    venc_attr.stVencAttr.u32PicWidth = FRAME_WIDTH;
    venc_attr.stVencAttr.u32PicHeight = FRAME_HEIGHT;
    venc_attr.stVencAttr.u32VirWidth = FRAME_WIDTH;
    venc_attr.stVencAttr.u32VirHeight = FRAME_HEIGHT;
    venc_attr.stVencAttr.u32StreamBufCnt = 8;

    // calculate size
    PIC_BUF_ATTR_S pic_buf_attr;
    MB_PIC_CAL_S mb_pic_cal;
    memset(&pic_buf_attr, 0, sizeof(PIC_BUF_ATTR_S));
    memset(&mb_pic_cal, 0, sizeof(MB_PIC_CAL_S));
    pic_buf_attr.u32Width = FRAME_WIDTH;
    pic_buf_attr.u32Height = FRAME_HEIGHT;
    pic_buf_attr.enPixelFormat = RK_VIDEO_FMT_YUV;
    ret = RK_MPI_CAL_COMM_GetPicBufferSize(&pic_buf_attr, &mb_pic_cal);
    venc_attr.stVencAttr.u32BufSize = mb_pic_cal.u32MBSize;
    success("calculate video size ok\n");

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = 10 * 1024;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = 60;

    ret = RK_MPI_VENC_CreateChn(VENC_CHANNEL, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "venc channel create error %x\n");
    }
    success("venc cannel create ok\n");

    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = mb_pic_cal.u32MBSize;
    memory_pool_config.u32MBCnt = 1;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    MB_POOL memory_pool = MB_INVALID_POOLID;
    memory_pool = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool == MB_INVALID_POOLID)
    {
        return error(RK_FAILURE, "memory pool create error");
    }
    success("memory pool create ok\n");

    // start recive frame
    VENC_RECV_PIC_PARAM_S receive_param;
    memset(&receive_param, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    receive_param.s32RecvPicNum = -1;
    ret = RK_MPI_VENC_StartRecvFrame(VENC_CHANNEL, &receive_param);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "start receive frame error %x\n");
    }
    success("start receive frame ok\n");

    // use memory
    MB_BLK block = RK_MPI_MB_GetMB(memory_pool, venc_attr.stVencAttr.u32BufSize, RK_TRUE);
    if (block == RK_NULL)
    {
        return error(RK_FAILURE, "get memory block error");
    }
    success("get memory block ok\n");

    void *vir_addr = RK_MPI_MB_Handle2VirAddr(block);

    // read
    FILE *f = fopen(INPUT_PATH, "r");
    ret = read_image_nv12(vir_addr, FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT, f);
    fclose(f);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "read image error %x\n");
    }
    success("read image ok\n");

    ret = RK_MPI_SYS_MmzFlushCache(block, RK_FALSE);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "memory flush cache error %x\n");
    }
    success("memory flush cache ok\n");

    VIDEO_FRAME_INFO_S frame;
    memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    frame.stVFrame.pMbBlk = block;
    frame.stVFrame.u32Width = FRAME_WIDTH;
    frame.stVFrame.u32Height = FRAME_HEIGHT;
    frame.stVFrame.u32VirWidth = FRAME_WIDTH;
    frame.stVFrame.u32VirHeight = FRAME_HEIGHT;
    frame.stVFrame.enPixelFormat = RK_FMT_YUV420SP;
    frame.stVFrame.u32FrameFlag = FRAME_FLAG_SNAP_END;

    // send frame
    ret = RK_MPI_VENC_SendFrame(VENC_CHANNEL, &frame, -1);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "send frame error %x\n");
    }
    success("send frame ok\n");

    // write
    FILE *d = fopen(OUPUT_PATH, "wb");

    void *data = RK_NULL;

    // get stream
    VENC_STREAM_S stream;
    stream.pstPack = malloc(sizeof(VENC_PACK_S));

    while (1)
    {
        ret = RK_MPI_VENC_GetStream(VENC_CHANNEL, &stream, -1);
        if (ret != RK_SUCCESS)
        {
            return error(ret, "get stream error %x\n");
        }
        success("get stream ok\n");

        // read data
        data = RK_MPI_MB_Handle2VirAddr(stream.pstPack->pMbBlk);
        success("handle 2 vir ok\n");

        // write to file
        fwrite(data, 1, stream.pstPack->u32Len, d);
        fflush(d);

        ret = RK_MPI_VENC_ReleaseStream(VENC_CHANNEL, &stream);
        if (ret != RK_SUCCESS)
        {
            return error(ret, "release stream error %x\n");
        }
        success("release stream ok\n");

        if (stream.pstPack->bStreamEnd == RK_TRUE)
        {
            break;
        }
    }

    // close
    if (stream.pstPack)
    {
        free(stream.pstPack);
    }
    fclose(d);

    // relase
    RK_MPI_MB_ReleaseMB(block);
    ret = RK_MPI_MB_ReleaseMB(block);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "release block error %x\n");
    }
    success("release block ok\n");

    ret = RK_MPI_VENC_StopRecvFrame(VENC_CHANNEL);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "stop receive frame error %x\n");
    }
    success("stop receive frame ok\n");

    ret = RK_MPI_MB_DestroyPool(memory_pool);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "destory memory pool error %x\n");
    }
    success("destory memory pool ok\n");

    ret = RK_MPI_VENC_DestroyChn(VENC_CHANNEL);
    if (ret != RK_SUCCESS)
    {
        return error(ret, "destory channel error %x\n");
    }
    success("destory channel ok\n");

    ret = RK_MPI_SYS_Exit();
    if (ret != RK_SUCCESS)
    {
        return error(ret, "exit error %x\n");
    }
    success("exit channel ok\n");

    return ret;
}
