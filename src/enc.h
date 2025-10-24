#ifndef RK_VENC_H
#define RK_VENC_H

int init_venc(unsigned int bit_rate, unsigned int gop, unsigned int width, unsigned int height, unsigned int buffer_count);

int start_venc(unsigned int width, unsigned int height);

int stop_venc();

int use_venc_data(void *data);

int close_venc();

#endif
