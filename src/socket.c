#include "socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include "utils.h"

typedef struct
{
    uint32_t id;
    uint32_t size;
    uint64_t timestamp;
    uint32_t reserved0;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t reserved6;
    uint32_t reserved7;
} socket_header_t;

#define SOCKET_HEADER_SIZE sizeof(socket_header_t)

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
    socket_header_t header = {
        .id = id,
        .size = size,
        .timestamp = time,
    };

    // send header
    ssize_t sent = send(fd, &header, SOCKET_HEADER_SIZE, MSG_NOSIGNAL);
    if (sent != SOCKET_HEADER_SIZE)
    {
        perror("socket send header error");
        return -1;
    }

    // send data
    ssize_t data_sent = 0;
    while (data_sent < size)
    {
        ssize_t st = send(fd, (char *)data + data_sent, size - data_sent, MSG_NOSIGNAL);
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
