#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DUMMY_FILE "dummy.bin"
#define MANIFEST_FILE "manifest.txt"

const char *databag_path;

void create_dummy_file(const char *path) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", path, DUMMY_FILE);
    
    int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("Failed to create dummy.bin");
        exit(1);
    }
    
    const char *dummy_content = "This is a dummy binary file.";
    write(fd, dummy_content, strlen(dummy_content));
    close(fd);
}

void create_manifest_file(const char *path) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", path, MANIFEST_FILE);
    
    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        perror("Failed to create manifest.txt");
        return;
    }
    
    fprintf(fp, "dummy.bin\n");
    fclose(fp);
}

void handle_signal(int signal) {
    create_manifest_file(databag_path);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[1], "record") != 0 || strcmp(argv[2], "start") != 0 || strcmp(argv[3], "--path") != 0) {
        fprintf(stderr, "Usage: %s record start --path <path>\n", argv[0]);
        exit(1);
    }

    databag_path = argv[4];

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    create_dummy_file(databag_path);

    // 模拟长时间运行的进程
    while (1) {
        sleep(1);
    }

    return 0;
}
