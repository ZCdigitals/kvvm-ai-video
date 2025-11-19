#ifndef VIDEO_H
#define VIDEO_H

#include <stdbool.h>
#include <stdint.h>

#include "rk_comm_video.h"
#include "rk_mpi_venc.h"

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
int calculate_venc(uint32_t width, uint32_t height, MB_PIC_CAL_S *mb_pic_cal);

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
int init_venc(int32_t channel_id, uint32_t width, uint32_t height, uint32_t bit_rate, uint32_t gop, uint32_t buffer_count, MB_PIC_CAL_S mb_pic_cal);

/**
 * init venc memory
 *
 * @param buffer_count buffer count, should be same as input frame buffer count
 * @param mb_pic_cal calculate result
 * @return memory pool id, MB_INVALID_POOLID error, should be [0, MB_MAX_COMM_POOLS)
 */
uint32_t init_venc_memory(uint32_t buffer_count, MB_PIC_CAL_S mb_pic_cal);

/**
 * destroy venc and venc memory
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param memory_pool_id memory pool, MB_INVALID_POOLID error, should be [0, MB_MAX_COMM_POOLS)
 */
void destroy_venc(int32_t channel_id, uint32_t memory_pool_id);

/**
 * start venc
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @return 0 ok, -1 error
 */
int start_venc(int32_t channel_id);

/**
 * stop venc
 *
 * @param channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @return 0 ok, -1 error
 */
int stop_venc(int32_t channel_id);

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
int allocate_buffers(uint32_t memory_pool_id, int video_fd, unsigned int buffer_count, void *blocks[]);

/**
 * free buffers
 *
 * @param buffer_count input frame buffer count
 * @param blocks block pointer array, eg. `void *blocks[BUFFER_COUNT]`
 * @return 0 ok, -1 error
 */
int free_buffers(unsigned int buffer_count, void *blocks[]);

/**
 * input
 *
 * read raw frame from v4l2 and send to venc
 *
 * @param venc_channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param video_fd device file description
 * @param width video width, eg. `1920`
 * @param height video height, eg. `1080`
 * @param vir_width stride width
 * @param vir_height stride height
 * @param is_end is end frame
 * @param timeout send frame timeout
 * @returns 0 ok, -1 error
 */
int input(int32_t venc_channel_id, int video_fd, uint32_t width, uint32_t height, uint32_t vir_width, uint32_t vir_height, bool is_end, int32_t timeout);

/**
 * output data callback
 * @param frame_id framd id
 * @param time time
 * @param data data pointer
 * @param size size
 * @param user_data context pointer
 * @returns 0 ok, -1 error
 */
typedef int (*output_data_callback)(uint32_t frame_id, uint64_t time, void *data, uint32_t size, void *user_data);

/**
 * output
 *
 * read encoded frame from venc stream
 *
 * @param venc_channel_id venc channel id, must be [0, VENC_MAX_CHN_NUM)
 * @param stream stream pointer
 * @param timeout get stream timeout
 * @param callback output data callback
 * @param callback_context output data callback context
 * @returns 0 ok, -1 error
 */
int output(int32_t venc_channel_id, VENC_STREAM_S *stream, int32_t timeout, output_data_callback callback, void *callback_context);

#endif
