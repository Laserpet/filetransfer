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

#define WATCH_DIR "/userdata/datalogger/tmp/"
#define TAR_FILE "/userdata/datalogger/tmp/tmp.tar"
#define CLIENT_PORT 6665
#define BUFFER_SIZE 1024 // 1KB
int error = 0;
// 函数声明
void create_tar();
void send_tar_to_client();

int main() {
    
    while (1) {
        // 检查 manifest.txt 文件是否存在
        struct stat statbuf;
        if (stat(WATCH_DIR"/manifest.txt", &statbuf) == 0) {
            printf("manifest.txt found, starting the process...\n");
            create_tar();
            send_tar_to_client();
        } else {
            printf("manifest.txt not found, waiting...\n");
        }
        // 每隔2秒检查一次
        sleep(2);
    }
    return 0;
}

void create_tar() {
    printf("Packing Files\n");
    char command[256];
    snprintf(command, sizeof(command), "tar -cf %s -C %s .", TAR_FILE, WATCH_DIR);
    system(command);
    printf("Created tar file: %s\n", TAR_FILE);
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

    FILE *tar_file;
    char buffer[BUFFER_SIZE];

    // 打开 tar 文件
    printf("打开 tar 文件\n");
    tar_file = fopen(TAR_FILE, "rb");
    printf("File opened\n");
    if (tar_file == NULL) {
        perror("fopen failed");
        close(sockfd);
        // 错误计数器+1
        error = error + 1;
    }

    // 发送 tar 文件
    while ((n = fread(buffer, 1, BUFFER_SIZE, tar_file)) > 0) {
        if (send(sockfd, buffer, n, 0) == -1) {
            perror("Error Sending File");
            // 错误计数器+1
            error = error + 10;
        }
    }

    fclose(tar_file);
    close(sockfd);

    // 删除原始文件
    if(error == 0){
    printf("tar file sent. Work Done in this cycle \n");
    system("rm -r /userdata/datalogger/tmp/*");
    printf("Removed all files in test\n");
    }else{
        error = 0;
    }
}
