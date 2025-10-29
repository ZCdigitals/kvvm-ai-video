#ifndef SOCKET_H
#define SCOKET_H

#include <stdint.h>

/**
 * init socket
 *
 * @param path file path, eg. `/tmp/stream.sock`
 * @param width frame width
 * @param height frame height
 * @return 0 ok, -1 error
 */
int init_socket(const char *path, uint32_t width, uint32_t height);

/**
 * send frame
 *
 * @param id frame id
 * @param time frame capture time
 * @param data frame data pointer
 * @param size frame data size
 * @return 0 ok, -1 error
 */
int send_frame(uint32_t id, uint64_t time, void *data, uint32_t size);

/**
 * close socket
 */
void close_socket();

#endif
