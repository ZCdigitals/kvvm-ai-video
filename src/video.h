#ifndef VIDEO_H
#define VIDEO_H

#include "rk_comm_video.h"

#define RK_COLOR RK_FMT_YUV420SP
#define V4L2_COLOR V4L2_PIX_FMT_NV12

/**
 * calculate venc size
 *
 * @param width video width, eg. `1920`
 * @param height video height, eg. `1080`
 * @param mb_pic_cal calculate result pointer
 * @return 0 ok, -1 error
 */
int calculate_venc(unsigned int width, unsigned int height, MB_PIC_CAL_S *mb_pic_cal);

/**
 * init venc
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param width video width, eg. `1920`
 * @param height video height, eg. `1080`
 * @param bit_rate bit rate, eg. `10 * 1024`
 * @param gop group of pictures, eg. `60`
 * @param buffer_count buffer count, this is output stream buffer count
 * @param mb_pic_cal calculate result
 * @return 0 ok, -1 error
 */
int init_venc(int channel_id, unsigned int width, unsigned int height, unsigned int bit_rate, unsigned int gop, unsigned int buffer_count, MB_PIC_CAL_S mb_pic_cal);

/**
 * init venc memory
 *
 * @param buffer_count buffer count, should be same as input frame buffer count
 * @param mb_pic_cal calculate result
 * @return memory pool id, MB_INVALID_POOLID error, should be [0, MB_MAX_COMM_POOLS)
 */
unsigned int init_venc_memory(unsigned int buffer_count, MB_PIC_CAL_S mb_pic_cal);

/**
 * destroy venc and venc memory
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param memory_pool_id memory pool, MB_INVALID_POOLID error, should be [0, MB_MAX_COMM_POOLS)
 */
void destroy_venc(int channel_id, unsigned int memory_pool_id);

/**
 * start venc
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @return 0 ok, -1 error
 */
int start_venc(int channel_id);

/**
 * stop venc
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @return 0 ok, -1 error
 */
int stop_venc(int channel_id);

/**
 * init v4l2
 *
 * @param path device path, eg. `/dev/video0`
 * @param width video width, eg. `1920`
 * @param height video height, eg. `1080`
 * @return device file description, -1 error
 */
int init_v4l2(const char *path, unsigned int width, unsigned int height);

/**
 * init v4l2 buffers
 *
 * @param fd device file description
 * @param buffer_count input frame buffer count
 * @return input frame buffer count, 0 error
 */
unsigned int init_v4l2_buffers(int fd, unsigned int buffer_count);

/**
 * destroy v4l2
 *
 * @param fd device file description
 */
void destroy_v4l2(int fd);

/**
 * start v4l2
 *
 * @param fd device file description
 * @return 0 ok, -1 error
 */
int start_v4l2(int fd);

/**
 * stop v4l2
 *
 * @param fd device file description
 * @return 0 ok, -1 error
 */
int stop_v4l2(int fd);

/**
 * allocate buffers
 *
 * @param memory_pool_id memory pool, MB_INVALID_POOLID error, should be [0, MB_MAX_COMM_POOLS)
 * @param video_fd device file description
 * @param buffer_count input frame buffer count
 * @param blocks block pointer array, eg. `void *blocks[BUFFER_COUNT]`
 * @return 0 ok, -1 error
 */
int allocate_buffers(unsigned int memory_pool_id, int video_fd, unsigned int buffer_count, void *blocks[]);

/**
 * free buffers
 *
 * @param buffer_count input frame buffer count
 * @param blocks block pointer array, eg. `void *blocks[BUFFER_COUNT]`
 * @return 0 ok, -1 error
 */
int free_buffers(unsigned int buffer_count, void *buffers[]);

#endif
