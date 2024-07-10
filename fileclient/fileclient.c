#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/statvfs.h>

#define PORT 6665
#define BUFFER_SIZE 1024
#define LOG_PATH "/userdata/domainlog/tmp.tar"
#define PORT_S1 6667
#define BUFFER_SIZE 1024

char domain_ip[BUFFER_SIZE] = "127.0.0.1";

int check_disk_space(){
    //等待补充

    return 0;
}

int receive_tar_file(char *argv[]) {
    char *directory_path = argv[0];
    char tmpfile[1024];
    snprintf(tmpfile, sizeof(tmpfile), "%s/tmp.tar", directory_path);
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
    fp = fopen(tmpfile, "wb");
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
    snprintf(new_filename, sizeof(new_filename), "%s/databaglog_%s.tar", directory_path, timestamp);

    // 检查文件是否存在以及路径是否正确
    if (access(tmpfile, F_OK) != 0) {
        perror("File to rename does not exist");
    } else {
        // 尝试重命名文件
        if (rename(tmpfile, new_filename) != 0) {
            perror("rename failed");
        } else {
            printf("Renamed tar file to: %s\n", new_filename);
        }
    }
    //重置响应标志位
    return 0;
}



int send_message(const char *ip, int port, const char *message) {
    int sockfd;
    struct sockaddr_in server_addr;
    int ret;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    send(sockfd, message, strlen(message), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
    } else {
        buffer[bytes_received] = '\0';
        printf("Received message on port %d: %s\n", PORT_S1, buffer);
        //如果接收到回应，则进行处理
        ret = response_handle(buffer);
        return ret;
    }
    return 0;
    close(sockfd);
}

int response_handle(char *buffer[BUFFER_SIZE]){
if (strcmp(buffer, "S1_ACK") == 0) {
    printf("Getting S1_ACK, STOP sending Starting MSG\n");
    return 1;
    }else {
    printf("Not Getting S1_ACK.\n");
    return 0;
    }
}

int is_directory_full(const char *path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0) {
        // 处理错误
        perror("statvfs");
        return -1;
    }

    // 检查可用空间是否为零
    if (stat.f_bavail == 0) {
        return 1; // 目录已满
    } else {
        return 0; // 目录未满
    }
}

int main(int argc, char *argv[]) {
    //参数解析
    char *domain_ip = argv[1];
    char *directory_path = argv[2];
    int ckdisk_result = is_directory_full(directory_path);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip_address> <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("Domain IP Address: %s\n", domain_ip);
    printf("Save Domainlog to: %s\n", directory_path);
   
    //开始主程序循环
    int resp_ret =0;
    while(1){
        //判断是否在等待文件传输回应，resp_ret==0时，不断发送S1，等待S1_ACK.
        if(resp_ret == 1){
        //此时已收到过S1_ACK,因此等待文件传入。   
            printf("Waiting for fileServer connection start\n");
            resp_ret = receive_tar_file(directory_path);
        }
        //如果
        while(resp_ret == 0){
        printf("Waiting for next connection\nSending Starting MSG S1\n");
        //当收到S1_ACK时，返回值为1，跳出while循环
        resp_ret = send_message(domain_ip, PORT_S1, "S1");
        sleep(1);
        }
    }
    return 0;
}
