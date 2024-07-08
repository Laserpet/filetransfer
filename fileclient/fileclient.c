#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT 6665
#define BUFFER_SIZE 1024
#define LOG_PATH "/userdata/domainlog/tmp.tar"

int check_disk_space(){
    //等待补充
    return 0;
}

void receive_tar_file() {
    int sockfd, new_sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    char buffer[BUFFER_SIZE];
    ssize_t n;
    FILE *fp;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // 绑定端口
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
    }

    // 监听端口
    if (listen(sockfd, 5) < 0) {
        perror("listen failed");
        close(sockfd);
    }

    // 接受客户端连接
    new_sockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
    if (new_sockfd < 0) {
        perror("accept failed");
        close(sockfd);
    }

    
    //检查磁盘空间是否满足要求
    if(check_disk_space() != 0){
        perror("Disk Space not enough");
        
    }
    // 打开文件以写入 tar 文件
    fp = fopen(LOG_PATH, "wb");
    if (fp == NULL) {
        perror("fopen failed");
        close(new_sockfd);
        close(sockfd);
        exit(1);
    }

    // 接收文件并写入文件
    while ((n = recv(new_sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, n, fp);
    }

    if (n < 0) {
        perror("recv failed");
    }

    printf("Received tar file\n");

    fclose(fp);
    close(new_sockfd);
    close(sockfd);

    // 改名为 databag+时间戳.tar
    time_t t;
    struct tm *tm_info;
    char new_filename[256];
    char timestamp[64];
    time(&t);
    tm_info = localtime(&t);
    printf("Changing File's name\n");
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);
    snprintf(new_filename, sizeof(new_filename), "/userdata/domainlog/databaglog_%s.tar", timestamp);


    // 检查文件是否存在以及路径是否正确
    if (access(LOG_PATH, F_OK) != 0) {
        perror("File to rename does not exist");
    } else {
        // 尝试重命名文件
        if (rename(LOG_PATH, new_filename) != 0) {
            perror("rename failed");
        } else {
            printf("Renamed tar file to: %s\n", new_filename);
        }
    }
}

int main() {
    while(1){
    receive_tar_file();
    }
    return 0;
}
