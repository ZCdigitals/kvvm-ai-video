#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stddef.h>

/**
 * init v4l2 device
 *
 * @param path file path, eg. `/dev/video0`
 * @param width video width
 * @param height video height
 * @return 0 ok, -1 error
 */
int init_v4l2(const char *path, int width, int height);

/**
 * start v4l2 caputre
 *
 * @return 0 ok, -1 error
 */
int start_v4l2_capture(void);

/**
 * caputre v4l2 frame
 *
 * @param frame_data_y output y data pointer pointer
 * @param frame_size_y output y size pointer
 * @param frame_data_uv output uv data pointer pointer
 * @param frame_size_uv output nv size pointer
 * @return buffer index, returns NULL when error
 */
unsigned int capture_v4l2_frame(void **frame_data_y, size_t *frame_size_y, void **frame_data_uv, size_t *frame_size_uv);

/**
 * stop v4l2 caputre
 *
 * @return 0 ok, -1 error
 */
int stop_v4l2_capture();

/**
 * close v4l2 device
 */
void close_v4l2();

#endif
