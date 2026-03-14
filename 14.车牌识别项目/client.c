#include "client.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

static char g_server_ip[32];
static int  g_server_port;

/* ===== 初始化网络 ===== */
int net_init(const char *server_ip, int port)
{
    strncpy(g_server_ip, server_ip, sizeof(g_server_ip)-1);
    g_server_ip[sizeof(g_server_ip)-1] = '\0';
    g_server_port = port;
    return 0;
}

/* ===== 发送车牌号 ===== */
int net_send_plate(const char *plate)
{
    int sockfd;
    struct sockaddr_in server_addr;

    /* 创建 socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    /* 配置服务器地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(g_server_port);
    server_addr.sin_addr.s_addr = inet_addr(g_server_ip);

    /* 连接服务器 */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    /* 发送车牌号 */
    send(sockfd, plate, strlen(plate), 0);

    close(sockfd);
    return 0;
}
