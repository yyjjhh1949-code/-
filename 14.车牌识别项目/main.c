#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "font.h"
#include "db.h"
#include "net.h"
#include "touch.h"

/* ========= LCD 参数 ========= */
#define LCD_DEV   "/dev/fb0"
#define LCD_W     800
#define LCD_H     480
#define LCD_BPP   4

extern int init_video();
extern int get_bmp(int fd);

static unsigned int *lcd_mem  = NULL;
static unsigned int *back_buf = NULL;

/* 状态定义 */
enum { MODE_MONITOR, MODE_ALL_LOGS, MODE_ONE_LOG };
int current_mode = MODE_MONITOR;

char current_plate[32] = {0}; // 当前识别到的车牌
char filter_plate[32] = {0};  // 要查询的车牌

/* ========= LCD 基础函数 ========= */
int lcd_init(void) {
    int fd = open(LCD_DEV, O_RDWR);
    if (fd < 0) return -1;
    lcd_mem = mmap(NULL, LCD_W*LCD_H*4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    back_buf = malloc(LCD_W*LCD_H*4);
    return 0;
}
void lcd_clear(unsigned int color) {
    for(int i=0; i<LCD_W*LCD_H; i++) back_buf[i] = color;
}
void lcd_flush(void) { memcpy(lcd_mem, back_buf, LCD_W*LCD_H*4); }

void lcd_show_bmp(const char *bmp) {
    int fd = open(bmp, O_RDONLY);
    if (fd < 0) return;
    lseek(fd, 54, SEEK_SET);
    unsigned char rgb[640*480*3];
    if(read(fd, rgb, sizeof(rgb)) > 0) {
        int start_x = (LCD_W - 640)/2;
        for (int y = 0; y < 480; y++) {
            unsigned int *dst = back_buf + y*LCD_W + start_x;
            unsigned char *src = rgb + (479-y)*640*3;
            for (int x = 0; x < 640; x++)
                dst[x] = (src[x*3+2]<<16) | (src[x*3+1]<<8) | src[x*3+0];
        }
    }
    close(fd);
}

void lcd_show_text(int x, int y, const char *text, unsigned int color) {
    font *f = fontLoad("/root/simfang.ttf");
    if (!f) return;
    fontSetSize(f, 32); 
    bitmap bm;
    bm.width = LCD_W;
    bm.height = LCD_H;
    bm.byteperpixel = 4;
    bm.map = (unsigned char *)back_buf;
    fontPrint(f, &bm, x, y, (char *)text, color, 0);
    fontUnload(f);
}

void draw_ui_button(int x, int y, int w, int h, char *text, unsigned int color) {
    // 画背景框
    for(int j=y; j<y+h; j++)
        for(int i=x; i<x+w; i++)
            if(i<LCD_W && j<LCD_H) back_buf[j*LCD_W+i] = color;
    // 画文字 (黑色)
    lcd_show_text(x+10, y+10, text, 0xFF000000);
}

/* ========= 坐标转换函数 (新增) ========= */
// 将触摸屏的 0-1024 映射到 LCD 的 0-800
void touch_to_lcd(int *x, int *y)
{
    // 这里假设你的触摸屏最大值是 1024 和 600 (根据你的日志推算)
    // 如果点击位置不准，可以微调这几个数字
    int raw_x = *x;
    int raw_y = *y;

    *x = raw_x * 800 / 1024;
    *y = raw_y * 480 / 600;
    
    // 防止越界
    if(*x > 800) *x = 800;
    if(*y > 480) *y = 480;
}

/* ========= 提取车牌 (超级调试版) ========= */
int get_plate(char *plate) {
    // 1. 先检查图片是否生成成功
    if(access("0.bmp", F_OK) != 0) {
        printf("错误: 0.bmp 文件不存在!\n");
        return -1;
    }

    FILE *fp = popen("alpr 0.bmp", "r");
    if (!fp) {
        printf("错误: 无法执行 alpr 命令\n");
        return -1;
    }

    char line[256];
    int found = -1;

    // printf("--- 开始识别 ---\n"); // 刷屏太快可以注释掉
    while (fgets(line, sizeof(line), fp)) {
        
        // 策略：只要包含 "confidence"
        if (strstr(line, "confidence")) {
            // 打印识别到的原始行，方便调试
            printf("[ALPR识别] %s", line); 

            // 尝试提取: 找 "confidence:" 后面的字符串，或者找 "- "
            // 兼容格式: "plate0: 10 results -- 92% confidence: 京A88888"
            // 兼容格式: "- 京A88888"
            
            // 方法：找最后一个空格，通常车牌在最后
            char *p = strrchr(line, ' '); 
            if(p && strlen(p) > 3) {
                strcpy(plate, p+1);
                // 去掉换行符
                if(plate[strlen(plate)-1] == '\n') plate[strlen(plate)-1] = 0;
                
                // 简单过滤：车牌长度通常大于6
                if(strlen(plate) >= 6) {
                    found = 0;
                    break; // 找到一个就退出
                }
            }
        }
    }
    pclose(fp);
    return found;
}

/* ========= 日志显示回调 ========= */
int log_y_pos = 60;
void show_log_item(int id, char *plate, char *time, int is_valid) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[%d] %s %s", id, plate, is_valid?"有效":"陌生");
    
    unsigned int color = is_valid ? 0xFF00FF00 : 0xFFFF0000;
    lcd_show_text(50, log_y_pos, buf, color);
    log_y_pos += 40;
}

/* ==================== 主函数 ==================== */
int main(void)
{
    lcd_init();
    db_init();
    net_init("172.9.1.156", 8888); 
    touch_init();
    int cam_fd = init_video();

    printf("程序启动... \n");

    while (1)
    {
        int tx = 0, ty = 0;
        int is_touched = get_touch(&tx, &ty);
        
        if(is_touched) {
            int raw_x = tx, raw_y = ty;
            // ⭐ 核心修改：进行坐标转换 ⭐
            touch_to_lcd(&tx, &ty);
            printf(">>> 触摸: 原始(%d,%d) -> LCD(%d,%d)\n", raw_x, raw_y, tx, ty);
        }

        /* ----------------- 模式1：监控 ----------------- */
        if (current_mode == MODE_MONITOR)
        {
            get_bmp(cam_fd);
            lcd_clear(0x00000000);
            lcd_show_bmp("0.bmp");

            // 识别
            char plate[32] = {0};
            int has_plate = 0;
            if (get_plate(plate) == 0)
            {
                has_plate = 1;
                int is_valid = check_plate_in_db(plate);
                
                // 存日志 (简单去重)
                static char last_plate[32] = {0};
                if(strcmp(last_plate, plate) != 0) {
                    db_insert_log(plate, is_valid);
                    net_send_plate_status(plate, is_valid);
                    strcpy(last_plate, plate);
                    printf(">>> 最终结果: %s (%s)\n", plate, is_valid?"有效":"陌生");
                }

                // 显示结果
                char text[64];
                snprintf(text, sizeof(text), "%s: %s", is_valid?"有效":"陌生", plate);
                lcd_show_text(200, 420, text, is_valid?0xFF00FF00:0xFFFF0000);

                // [该车记录] 按钮 (左下角)
                draw_ui_button(20, 400, 200, 60, "该车记录", 0xFFFFFF00); 
                strcpy(current_plate, plate); 
            }
            else {
                lcd_show_text(300, 420, "监控中...", 0xFFFFFFFF);
            }

            // [所有记录] 按钮 (右下角)
            draw_ui_button(580, 400, 200, 60, "所有记录", 0xFF00FFFF);

            // 触摸处理
            if (is_touched) {
                // 点击 [所有记录] (580,400) - (780,460)
                if (tx > 580 && tx < 780 && ty > 400 && ty < 460) {
                    printf("点击了 [所有记录]\n");
                    current_mode = MODE_ALL_LOGS;
                    usleep(200000);
                }
                // 点击 [该车记录] (20,400) - (220,460)
                if (has_plate && tx > 20 && tx < 220 && ty > 400 && ty < 460) {
                    printf("点击了 [该车记录]\n");
                    strcpy(filter_plate, current_plate); 
                    current_mode = MODE_ONE_LOG;
                    usleep(200000);
                }
            }
        }
        /* ----------------- 模式2：所有记录 ----------------- */
        else if (current_mode == MODE_ALL_LOGS)
        {
            lcd_clear(0xFF222222); 
            lcd_show_text(300, 10, "== 所有记录 ==", 0xFFFFFFFF);
            
            log_y_pos = 60;
            db_get_all_logs(show_log_item);

            // [返回] 按钮
            draw_ui_button(600, 400, 150, 60, "返回", 0xFFCCCCCC);

            if (is_touched) {
                if (tx > 600 && tx < 750 && ty > 400 && ty < 460) {
                    printf("点击了 [返回]\n");
                    current_mode = MODE_MONITOR;
                }
            }
            lcd_flush();
            usleep(100000); 
        }
        /* ----------------- 模式3：单车记录 ----------------- */
        else if (current_mode == MODE_ONE_LOG)
        {
            lcd_clear(0xFF222222);
            char title[64];
            snprintf(title, sizeof(title), "记录: %s", filter_plate);
            lcd_show_text(250, 10, title, 0xFFFFFFFF);
            
            log_y_pos = 60;
            db_get_logs_by_plate(filter_plate, show_log_item);

            // [返回] 按钮
            draw_ui_button(600, 400, 150, 60, "返回", 0xFFCCCCCC);

            if (is_touched) {
                if (tx > 600 && tx < 750 && ty > 400 && ty < 460) {
                    current_mode = MODE_MONITOR;
                }
            }
            lcd_flush();
            usleep(100000);
        }

        lcd_flush();
        if(current_mode == MODE_MONITOR) usleep(10000);
    }
    return 0;
}