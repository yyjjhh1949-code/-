#ifndef _CLIENT_H_
#define _CLIENT_H_

int net_init(const char *server_ip, int port);
int net_send_plate(const char *plate);

#endif
