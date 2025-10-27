#ifndef SOCKET_H
#define SCOKET_H

#include <stdint.h>

int init_socket(const char *path);

int send_socket(uint32_t width, uint32_t height, void *data, uint32_t size);

int close_socket();

#endif
