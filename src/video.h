#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

/**
 * init v4l2 device
 *
 * @param path file path, eg. `/dev/video0`
 * @param width video width
 * @param height video height
 * @param buffer_count buffer count
 * @return buffer count, 0 is error
 */
unsigned int init_v4l2(const char *path, int width, int height, unsigned int buffer_count);

unsigned int init_v4l2_buffer(unsigned int index);
int use_v4l2_buffer(int plane_fd);

/**
 * start v4l2 caputre
 *
 * @return 0 ok, -1 error
 */
int start_v4l2_capture();

int capture_v4l2_frame(unsigned int *id, unsigned long long int *time);
int release_v4l2_frame();

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
