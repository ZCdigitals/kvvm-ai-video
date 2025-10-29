#include <errno.h>
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

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080

#define BIT_RATE 10 * 1024
#define GOP 2

int main()
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
    venc_attr.stVencAttr.u32PicWidth = VIDEO_WIDTH;
    venc_attr.stVencAttr.u32PicHeight = VIDEO_HEIGHT;
    venc_attr.stVencAttr.u32VirWidth = VIDEO_WIDTH;
    venc_attr.stVencAttr.u32VirHeight = VIDEO_HEIGHT;
    venc_attr.stVencAttr.u32StreamBufCnt = 8;

    // calculate size
    PIC_BUF_ATTR_S pic_buf_attr;
    MB_PIC_CAL_S mb_pic_cal;
    memset(&pic_buf_attr, 0, sizeof(PIC_BUF_ATTR_S));
    memset(&mb_pic_cal, 0, sizeof(MB_PIC_CAL_S));
    pic_buf_attr.u32Width = VIDEO_WIDTH;
    pic_buf_attr.u32Height = VIDEO_HEIGHT;
    pic_buf_attr.enPixelFormat = RK_VIDEO_FMT_YUV;
    ret = RK_MPI_CAL_COMM_GetPicBufferSize(&pic_buf_attr, &mb_pic_cal);
    venc_attr.stVencAttr.u32BufSize = mb_pic_cal.u32MBSize;
    printf("mpi calculate ok\n");

    // set h264 struct props
    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = BIT_RATE;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = GOP;

    ret = RK_MPI_VENC_CreateChn(VENC_CHANNEL, &venc_attr);
    if (ret != RK_SUCCESS)
    {
        errno = ret;
        perror("mpi venc channel create error");
        return -1;
    }
    printf("mpi venc channel create ok\n");

    ret = RK_MPI_VENC_DestroyChn(VENC_CHANNEL);
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
