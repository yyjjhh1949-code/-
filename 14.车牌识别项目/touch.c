#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "touch.h"

static int touch_fd = -1;

int touch_init(void)
{
    // 注意：根据你的开发板型号，可能是 event0, event1 等
    // GEC6818 通常是 event0
    touch_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK); // 非阻塞模式
    if (touch_fd < 0) {
        perror("open touch");
        return -1;
    }
    return 0;
}

int get_touch(int *x, int *y)
{
    struct input_event ev;
    static int cur_x = -1, cur_y = -1;
    int pressed = 0;

    // 读取所有堆积的事件
    while (read(touch_fd, &ev, sizeof(ev)) > 0)
    {
        if (ev.type == EV_ABS)
        {
            if (ev.code == ABS_X) cur_x = ev.value;
            if (ev.code == ABS_Y) cur_y = ev.value;
        }
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
        {
            if (ev.value == 1) { // 按下
                // 此时坐标可能还没更新完，通常等待弹起或持续更新
            } else if (ev.value == 0) { // 弹起 (认为是一次点击)
                if(cur_x != -1 && cur_y != -1) {
                    *x = cur_x; // 这里的坐标可能需要校准，视屏幕而定
                    *y = cur_y; // GEC6818通常不需要复杂校准，直接用
                    // 有些屏幕需要 x = x * 800 / 1024 之类的转换
                    pressed = 1;
                }
            }
        }
    }
    return pressed;
}