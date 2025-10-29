#ifndef RK_VENC_H
#define RK_VENC_H

#include <stdbool.h>

/**
 * init venc
 *
 * @param bit_rate bit rate
 * @param gop group of picture
 * @param width video width
 * @param height video height
 * @return 0 ok, -1 error
 */
int init_venc(unsigned int bit_rate, unsigned int gop, unsigned int width, unsigned int height);

/**
 * start venc
 *
 * @return 0 ok, -1 error
 */
int start_venc();

int use_venc_frame(unsigned int size);
int send_venc_frame(int fd, unsigned int id, unsigned long long int time, bool is_end);
int free_venc_frame(int fd);

int read_venc_frame(void **dst, unsigned int *size);
int release_venc_frame();

/**
 * stop venc
 */
void stop_venc();

/**
 * close venc
 */
void close_venc();

#endif
