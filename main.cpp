#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>        // strerror
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

#define SERVER_PORT 5000


int main_t(void) {

    int ret = 0;
    int sock;	// 通信套接字
    struct sockaddr_in server_addr;

    // 1.创建通信套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock) {
        fprintf(stderr, "create socket error, reason: %s\n", strerror(errno));
        exit(-1);
    }

    // 2.清空标签，写上地址和端口号
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;	// 选择协议组ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	// 监听本地所有IP地址
    server_addr.sin_port = htons(SERVER_PORT);			// 绑定端口号

    // 3.绑定
    ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (-1 == ret) {
        fprintf(stderr, "socket bind error, reason: %s\n", strerror(errno));
        close(sock);
        exit(-2);
    }

    // 4.监听，同时监听128个请求
    ret = listen(sock, 128);
    if (-1 == ret) {
        fprintf(stderr, "listen error, reason: %s\n", strerror(errno));
        close(sock);
        exit(-2);
    }

    printf("等待客户端的链接\n");

    int done = 1;

    while (done) {

        struct sockaddr_in client;
        int client_sock;
        char client_ip[64];
        int len = 0;
        char buf[256];

        socklen_t client_addr_len;
        client_addr_len = sizeof(client);
        // 5.接受
        client_sock = accept(sock, (struct sockaddr *)&client, &client_addr_len);
        if (-1 == client_sock) {
            perror("accept error");
            close(sock);
            exit(-3);
        }

        // 打印客户端IP地址和端口号
        printf("client ip: %s\t port: %d\n",
                inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                ntohs(client.sin_port));


        // 6.读取客户端发送的数据
//        len = read(client_sock, buf, sizeof(buf)-1);
        len = recv(client_sock, buf, sizeof(buf), 0);
        if (-1 == len) {
            perror("read error");
            close(sock);
            close(client_sock);
            exit(-4);
        }

        buf[len] = '\0';
        printf("recive[%d]: %s\n", len, buf);

        // 7.给客户端发送数据
        len = send(client_sock, buf, len, 0);
        if (-1 == len) {
            perror("write error");
            close(sock);
            close(client_sock);
            exit(-5);
        }

        printf("write finished. len: %d\n", len);
        // 8.关闭客户端套接字
        close(client_sock);
    }

    // 9.关闭服务器套接字
    close(sock);

    return 0;
}
