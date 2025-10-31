#ifndef SOCKET_H
#define SCOKET_H

#include <stdint.h>

/**
 * init socket
 *
 * @param path file path, eg. `/var/run/capture.sock`
 * @return socket file description, -1 error
 */
int init_socket(const char *path);

/**
 * send frame
 *
 * @param fd socket file description
 * @param id frame id
 * @param time frame capture time
 * @param width frame width
 * @param height frame height
 * @param data frame data pointer
 * @param size frame data size
 * @return 0 ok, -1 error
 */
int send_frame(int fd, uint32_t id, uint64_t time, uint32_t width, uint32_t height, void *data, uint32_t size);

/**
 * destroy socket
 *
 * @param fd socket file description
 */
void destroy_socket(int fd);

#endif
