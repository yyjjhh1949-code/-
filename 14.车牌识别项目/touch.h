#ifndef __TOUCH_H__
#define __TOUCH_H__

// 初始化触摸屏
int touch_init(void);
// 获取点击坐标 (返回 0 表示未点击，1 表示点击，坐标存入 x, y)
// 这是一个非阻塞检查，适合放在循环里
int get_touch(int *x, int *y);

#endif