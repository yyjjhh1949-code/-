#ifndef __NET_H__
#define __NET_H__

void net_init(const char *ip, int port);
void net_send_plate_status(const char *plate, int is_valid); // 新增声明

#endif

