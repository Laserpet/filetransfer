#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT_S1 6667
#define PORT_S2 6668
#define BUFFER_SIZE 1024

void send_message(const char *ip, int port, const char *message) {
    int sockfd;
    struct sockaddr_in server_addr;

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
        printf("Received response: %s\n", buffer);
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <1|2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int option = atoi(argv[1]);

    if (option == 1) {
        printf("Sending S1 message to port %d...\n", PORT_S1);
        send_message("192.168.9.100", PORT_S1, "S1");
    } else if (option == 2) {
        printf("Sending S2 message to port %d...\n", PORT_S2);
        send_message("192.168.9.100", PORT_S2, "S2");
    } else {
        fprintf(stderr, "Invalid option. Use 1 to send S1 message and 2 to send S2 message.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
