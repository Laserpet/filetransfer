#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#define PORT_S1 6667
#define PORT_S2 6668
#define BUFFER_SIZE 1024

int databag_pid = 0;  // 用于存储 databag 进程的 PID

// 发送消息的函数
void send_message(int sockfd, const char *message) {
    send(sockfd, message, strlen(message), 0);
}

// 处理 S1 消息的函数
void handle_s1(int sockfd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("recv failed or connection closed");
        close(sockfd);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received message on port %d: %s\n", PORT_S1, buffer);

    if (strcmp(buffer, "S1") == 0) {
        if (databag_pid != 0) {
            printf("databag process is already running.\n");
        } else {
            printf("Handling S1 message.\n");
            send_message(sockfd, "S1_ACK");

            // 执行命令
            printf("Starting databag process...\n");
            databag_pid = fork();
            if (databag_pid == 0) {
                execl("/userdata/datalogger/databag", "databag", "record", "start", "--path", "/userdata/datalogger/tmp", (char *)NULL);
                perror("execl failed");
                exit(1);
            } else if (databag_pid < 0) {
                perror("fork failed");
            }
        }
    }
    close(sockfd);
}

// 处理 S2 消息的函数
void handle_s2(int sockfd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("recv failed or connection closed");
        close(sockfd);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received message on port %d: %s\n", PORT_S2, buffer);

    if (strcmp(buffer, "S2") == 0) {
        if (databag_pid == 0) {
            printf("No databag process to kill.\n");
        } else {
            printf("Handling S2 message.\n");
            send_message(sockfd, "S2_ACK");

            // 杀掉 databag 进程
            printf("Killing databag process...\n");
            kill(databag_pid, SIGTERM);
            waitpid(databag_pid, NULL, 0);  // 等待子进程结束
            databag_pid = 0;
        }
    }

    close(sockfd);
}

int main() {
    int sockfd_s1, sockfd_s2, newsockfd_s1, newsockfd_s2;
    struct sockaddr_in server_addr_s1, server_addr_s2, client_addr_s1, client_addr_s2;
    socklen_t client_len_s1, client_len_s2;
    fd_set readfds;
    int maxfd;

    // 创建 socket
    sockfd_s1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_s1 < 0) {
        perror("socket creation for S1 failed");
        exit(EXIT_FAILURE);
    }

    sockfd_s2 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_s2 < 0) {
        perror("socket creation for S2 failed");
        close(sockfd_s1);
        exit(EXIT_FAILURE);
    }

    // 绑定和监听 6667 端口
    memset(&server_addr_s1, 0, sizeof(server_addr_s1));
    server_addr_s1.sin_family = AF_INET;
    server_addr_s1.sin_addr.s_addr = INADDR_ANY;
    server_addr_s1.sin_port = htons(PORT_S1);

    if (bind(sockfd_s1, (struct sockaddr *)&server_addr_s1, sizeof(server_addr_s1)) < 0) {
        perror("bind failed for S1");
        close(sockfd_s1);
        close(sockfd_s2);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd_s1, 5) < 0) {
        perror("listen failed for S1");
        close(sockfd_s1);
        close(sockfd_s2);
        exit(EXIT_FAILURE);
    }

    // 绑定和监听 6668 端口
    memset(&server_addr_s2, 0, sizeof(server_addr_s2));
    server_addr_s2.sin_family = AF_INET;
    server_addr_s2.sin_addr.s_addr = INADDR_ANY;
    server_addr_s2.sin_port = htons(PORT_S2);

    if (bind(sockfd_s2, (struct sockaddr *)&server_addr_s2, sizeof(server_addr_s2)) < 0) {
        perror("bind failed for S2");
        close(sockfd_s1);
        close(sockfd_s2);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd_s2, 5) < 0) {
        perror("listen failed for S2");
        close(sockfd_s1);
        close(sockfd_s2);
        exit(EXIT_FAILURE);
    }

    printf("Listening on ports %d and %d...\n", PORT_S1, PORT_S2);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd_s1, &readfds);
        FD_SET(sockfd_s2, &readfds);
        maxfd = (sockfd_s1 > sockfd_s2) ? sockfd_s1 : sockfd_s2;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            close(sockfd_s1);
            close(sockfd_s2);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(sockfd_s1, &readfds)) {
            client_len_s1 = sizeof(client_addr_s1);
            newsockfd_s1 = accept(sockfd_s1, (struct sockaddr *)&client_addr_s1, &client_len_s1);
            if (newsockfd_s1 < 0) {
                perror("accept failed for S1");
                continue;
            }
            handle_s1(newsockfd_s1);
        }

        if (FD_ISSET(sockfd_s2, &readfds)) {
            client_len_s2 = sizeof(client_addr_s2);
            newsockfd_s2 = accept(sockfd_s2, (struct sockaddr *)&client_addr_s2, &client_len_s2);
            if (newsockfd_s2 < 0) {
                perror("accept failed for S2");
                continue;
            }
            handle_s2(newsockfd_s2);
        }
    }

    close(sockfd_s1);
    close(sockfd_s2);
    return 0;
}
