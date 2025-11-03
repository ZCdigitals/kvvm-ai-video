#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <fcntl.h>
#include <unistd.h>

#define VIDEO_PATH "/dev/v4l-subdev2"

int main()
{
    printf("video deivce %s\n", VIDEO_PATH);

    int fd = open(VIDEO_PATH, O_RDWR);
    if (fd == -1)
    {
        perror("video deivce open error");
        return -1;
    }

    struct v4l2_dv_timings timings;
    memset(&timings, 0, sizeof(timings));

    int ret = ioctl(fd, VIDIOC_QUERY_DV_TIMINGS, &timings);
    if (ret != 0)
    {
        if (errno == ENOLINK)
        {
            printf("No signal\n");
            return -1;
        }
        else if (errno == ENOLCK)
        {
            printf("No lock\n");
            return -1;
        }
        else if (errno == ERANGE)
        {
            printf("Out of range");
            return -1;
        }
        else
        {
            perror("video device query timings error");
            return -1;
        }
    }

    printf("video width  %d\n", timings.bt.width);
    printf("video height %d\n", timings.bt.height);

    close(fd);

    return 0;
}
