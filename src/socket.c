#include "socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <unistd.h>

#include "utils.h"

typedef struct
{
    uint32_t id;
    uint32_t width;
    uint32_t height;
    /**
     * - 0, nv12
     *
     * only support nv12
     */
    uint32_t format;
    // in ms
    uint64_t time;
    uint32_t size;
    uint32_t reserved;
} frame_header;

int init_socket(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket error");
        return -1;
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);

    int ret = connect(fd, (struct sockaddr *)&address, sizeof(address));
    if (ret == -1)
    {
        perror("socket connect error");
        close(fd);
        fd = -1;
        return -1;
    }

    return fd;
}

int send_frame(int fd, uint32_t id, uint64_t time, uint32_t width, uint32_t height, void *data, uint32_t size)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    frame_header header = {
        .id = id,
        .width = width,
        .height = height,
        .format = 0,
        .time = time,
        .size = size,
        .reserved = 0,
    };

    // send header
    ssize_t sent = send(fd, &header, sizeof(header), MSG_NOSIGNAL);
    if (sent != sizeof(header))
    {
        perror("socket send header error");
        return -1;
    }

    // send data
    uint32_t data_sent = 0;
    while (data_sent < size)
    {
        uint32_t st = send(fd, (char *)data + data_sent, size - data_sent, MSG_NOSIGNAL);
        if (st == -1)
        {
            perror("socket send data error");
            return -1;
        }

        data_sent += st;
    }

    return 0;
}

void destroy_socket(int fd)
{
    close(fd);
}
