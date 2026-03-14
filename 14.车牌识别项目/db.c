#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "db.h"

static sqlite3 *db = NULL;

int db_init(void)
{
    int rc = sqlite3_open("car.db", &db);
    if (rc) return -1;

    // 保证表存在
    char *sql1 = "CREATE TABLE IF NOT EXISTS car_msg (plate TEXT PRIMARY KEY);";
    sqlite3_exec(db, sql1, 0, 0, 0);

    char *sql2 = "CREATE TABLE IF NOT EXISTS car_log ("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "plate TEXT, "
                 "time TEXT, "
                 "valid INTEGER);";
    sqlite3_exec(db, sql2, 0, 0, 0);
    return 0;
}

int check_plate_in_db(char *plate)
{
    char sql[128];
    snprintf(sql, sizeof(sql), "SELECT * FROM car_msg WHERE plate='%s';", plate);
    char **result;
    int row, col, ret = 0;
    if (sqlite3_get_table(db, sql, &result, &row, &col, NULL) == SQLITE_OK) {
        if (row > 0) ret = 1;
        sqlite3_free_table(result);
    }
    return ret;
}

void db_insert_log(char *plate, int is_valid)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);

    char sql[256];
    snprintf(sql, sizeof(sql), 
             "INSERT INTO car_log (plate, time, valid) VALUES ('%s', '%s', %d);", 
             plate, time_str, is_valid);
    sqlite3_exec(db, sql, 0, 0, 0);
}

// 内部通用查询函数
void db_query_generic(char *sql, LogCallback callback)
{
    char **result;
    int row, col;
    if (sqlite3_get_table(db, sql, &result, &row, &col, NULL) == SQLITE_OK)
    {
        // 只显示最近 8 条，防止超出屏幕
        int start = 1;
        if(row > 8) start = row - 8 + 1; // 简单的滚动逻辑：只看最后8条

        for (int i = start; i <= row; i++)
        {
            int id = atoi(result[i * col + 0]);
            char *p = result[i * col + 1];
            char *t = result[i * col + 2];
            int v = atoi(result[i * col + 3]);
            if(callback) callback(id, p, t, v);
        }
        sqlite3_free_table(result);
    }
}

void db_get_all_logs(LogCallback callback)
{
    // 按ID倒序，最近的在上面
    char *sql = "SELECT id, plate, time, valid FROM car_log ORDER BY id DESC LIMIT 8;";
    db_query_generic(sql, callback);
}

void db_get_logs_by_plate(char *target_plate, LogCallback callback)
{
    char sql[128];
    snprintf(sql, sizeof(sql), 
             "SELECT id, plate, time, valid FROM car_log WHERE plate='%s' ORDER BY id DESC LIMIT 8;", 
             target_plate);
    db_query_generic(sql, callback);
}