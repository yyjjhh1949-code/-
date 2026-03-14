/* server.c - 运行在虚拟机 Ubuntu 上 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8888

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // 1. 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT);

    // 2. 绑定
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. 监听
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf(">>> 服务器已启动，正在监听端口 %d <<<\n", PORT);

    while(1)
    {
        // 4. 等待连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // 获取客户端IP
        char *client_ip = inet_ntoa(address.sin_addr);
        printf("✅ 客户端已连接: %s\n", client_ip);

        // 5. 读取数据
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, 1024);
        
        if (valread > 0)
        {
            // 协议格式约定： "车牌号:状态"
            char plate[64] = {0};
            int status = 0;
            
            // 解析数据
            char *token = strtok(buffer, ":"); // 获取冒号前的车牌
            if(token != NULL) {
                strcpy(plate, token);
                token = strtok(NULL, ":");     // 获取冒号后的状态
                if(token != NULL) {
                    status = atoi(token);
                }
            }

            // 6. 打印中文结果
            printf("   ----------------------\n");
            printf("   收到车牌: %s\n", plate);
            
            if (status == 1) {
                printf("   识别结果: [ ✅ 有效车牌 ]\n"); 
            } else {
                printf("   识别结果: [ ⚠️  陌生车牌 ]\n"); 
            }
            printf("   ----------------------\n\n");
        }

        // 7. 关闭当前连接
        close(new_socket);
    }
    return 0;
}