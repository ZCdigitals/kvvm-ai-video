#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 初始化V4L2设备
     *
     * @param path 设备路径，如 `/dev/video0`
     * @param width 图像宽度
     * @param height 图像高度
     * @return int 成功返回文件描述符，失败返回-1
     */
    int init_v4l2(const char *path, int width, int height);

    /**
     * @brief 开始视频采集
     *
     * @return int 成功返回0，失败返回-1
     */
    int start_v4l2_capture(void);

    /**
     * @brief 捕获一帧图像
     *
     * @param frame_data_y 输出参数，指向帧y数据的指针
     * @param frame_size_y 输出参数，帧数据y大小
     * @param frame_data_uv 输出参数，指向帧uv数据的指针
     * @param frame_size_uv 输出参数，帧数据uv大小
     * @return int 成功返回缓冲区索引，失败返回-1
     */
    int capture_v4l2_frame(void **frame_data_y, size_t *frame_size_y, void **frame_data_uv, size_t *frame_size_uv);

    /**
     * @brief 停止视频采集
     */
    void stop_v4l2_capture(void);

    /**
     * @brief 关闭V4L2设备并释放资源
     */
    void close_v4l2(void);

#ifdef __cplusplus
}
#endif

#endif
