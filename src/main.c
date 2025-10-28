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
    return 0;
}
