#ifndef SOCKET_H
#define SCOKET_H

#include <stdint.h>

/**
 * init socket
 *
 * @param path file path, eg. `/tmp/stream.sock`
 * @return 0 ok, -1 error
 */
int init_socket(const char *path);

/**
 * send frame
 *
 * @param width frame width
 * @param height frame height
 * @param data frame data pointer
 * @param size frame data size
 * @return 0 ok, -1 error
 */
int send_frame(uint32_t width, uint32_t height, void *data, uint32_t size);

/**
 * close socket
 */
void close_socket();

#endif
