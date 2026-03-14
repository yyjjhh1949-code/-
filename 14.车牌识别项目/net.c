/* net.c 修改版 - 带调试信息 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "net.h"

static int sock_fd = -1;
static char server_ip[32];
static int server_port;

void net_init(const char *ip, int port)
{
    strcpy(server_ip, ip);
    server_port = port;
    printf("网络初始化: 目标IP=%s, 端口=%d\n", server_ip, server_port);
}

void net_send_plate_status(const char *plate, int is_valid)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("❌ 创建socket失败");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("❌ IP地址转换失败: %s\n", server_ip);
        close(sockfd);
        return;
    }

    // 设置连接超时 (可选，防止卡死)
    struct timeval timeout;
    timeout.tv_sec = 3;  // 3秒超时
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // 尝试连接
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("❌ 连接服务器失败"); // 这里会打印具体的错误原因，如 Connection refused
        close(sockfd);
        return;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "%s:%d", plate, is_valid);

    int ret = write(sockfd, buf, strlen(buf));
    if (ret < 0) {
        perror("❌ 发送数据失败");
    } else {
        printf("✅ 已发送: %s\n", buf); // 在开发板终端打印发送成功
    }

    close(sockfd);
}