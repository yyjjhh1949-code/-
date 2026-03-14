#ifndef __DB_H__
#define __DB_H__

int db_init(void);
int check_plate_in_db(char *plate);
void db_insert_log(char *plate, int is_valid);

// 回调函数定义
typedef void (*LogCallback)(int id, char *plate, char *time, int is_valid);

// 获取所有日志
void db_get_all_logs(LogCallback callback);

// 获取指定车牌的日志 (新增功能)
void db_get_logs_by_plate(char *target_plate, LogCallback callback);

#endif