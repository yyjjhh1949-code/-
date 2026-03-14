#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

int main()
{
    // 如果 event0 没反应，试着改成 event1 或 event2
    int fd = open("/dev/input/event0", O_RDONLY);
    if (fd < 0) {
        perror("打开触摸屏失败");
        return -1;
    }
    printf("触摸屏打开成功，请点击屏幕...\n");

    struct input_event ev;
    while (1)
    {
        if (read(fd, &ev, sizeof(ev)) > 0)
        {
            if (ev.type == EV_ABS)
            {
                if (ev.code == ABS_X) printf("X轴原始坐标: %d\n", ev.value);
                if (ev.code == ABS_Y) printf("Y轴原始坐标: %d\n", ev.value);
            }
            if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
            {
                if(ev.value == 1) printf("--- 按下 ---\n");
                if(ev.value == 0) printf("--- 松开 ---\n");
            }
        }
    }
    return 0;
}