#include <signal.h>

#define VIDEO_PATH "/dev/video0"
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define OUTPUT_PATH "/tmp/capture"

// 全局标志位，用于控制主循环
static volatile int keep_running = 1;

void main_stop()
{
    keep_running = 0;
}

int main()
{
    // regist signal
    signal(SIGINT, main_stop);
    signal(SIGTERM, main_stop);

    while (keep_running){
        
    }

    return 0;
}
