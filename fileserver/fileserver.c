#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <time.h>

#define CLIENT_PORT 6665
#define BUFFER_SIZE 1024 // 1KB
int error = 0;
char watch_dir[BUFFER_SIZE];
char tar_file[BUFFER_SIZE];

// 函数声明
void create_tar();
void send_tar_to_client();

int main(int argc, char *argv[]) {
    if (argc > 1) {
        strncpy(watch_dir, argv[1], BUFFER_SIZE - 1);
        watch_dir[BUFFER_SIZE - 1] = '\0';
        snprintf(tar_file, BUFFER_SIZE, "%s/tmp.tar", watch_dir);
    } else {
        printf("Usage: %s <watch_directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // 检查 Manifest.bin 文件是否存在
        struct stat statbuf;
        char manifest_path[BUFFER_SIZE];
        snprintf(manifest_path, BUFFER_SIZE, "%s/Manifest.bin", watch_dir);

        if (stat(manifest_path, &statbuf) == 0) {
            printf("Manifest.bin found, starting the process...\n");
            create_tar();
            send_tar_to_client();
        } else {
            printf("Manifest.bin not found, waiting...\n");
        }
        // 每隔2秒检查一次
        sleep(2);
    }
    return 0;
}

void create_tar() {
    printf("Packing Files\n");
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "tar -cf %s -C %s .", tar_file, watch_dir);
    system(command);
    printf("Created tar file: %s\n", tar_file);
}

void send_tar_to_client() {
    int sockfd;
    struct sockaddr_in servaddr;
    size_t n;
    printf("Sending tar files to client..\n");
    // 创建套接字
    printf("Creating Sockets\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        // 错误计数器+1
        error = error + 1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.9.81");
    servaddr.sin_port = htons(CLIENT_PORT);

    // 连接到客户端
    printf("Connecting Now\n");
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect failed");
        close(sockfd);
        // 错误计数器+1
        error = error + 1;
    }

    FILE *tar_file_fp;
    char buffer[BUFFER_SIZE];

    // 打开 tar 文件
    printf("打开 tar 文件\n");
    tar_file_fp = fopen(tar_file, "rb");
    printf("File opened\n");
    if (tar_file_fp == NULL) {
        perror("fopen failed");
        close(sockfd);
        // 错误计数器+1
        error = error + 1;
    }

    // 发送 tar 文件
    while ((n = fread(buffer, 1, BUFFER_SIZE, tar_file_fp)) > 0) {
        if (send(sockfd, buffer, n, 0) == -1) {
            perror("Error Sending File");
            // 错误计数器+1
            error = error + 10;
        }
    }

    fclose(tar_file_fp);
    close(sockfd);

    // 删除原始文件
    if(error == 0){
    printf("tar file sent. Work Done in this cycle \n");
    char rm_command[BUFFER_SIZE];
    snprintf(rm_command, BUFFER_SIZE, "rm -r %s/*", watch_dir);
    system(rm_command);
    printf("Removed all files in %s\n", watch_dir);
    } else {
        error = 0;
    }
}
