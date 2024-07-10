#include "TcpService.h"
#include <unistd.h>
#include <sys/types.h>
#ifdef MINGW32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>

//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

// #pragma comment(lib, "Ws2_32.lib")

TcpServer::TcpServer() {
#if 0
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return;
    }
#endif
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        printf("Error  m_fd: %d\n", m_fd);
        // WSACleanup();
        // return;
    } else {
        printf("TcpServer: m_fd: %d\n", m_fd);
    }
}

TcpServer::~TcpServer() {
    //closesocket(m_fd);
    close(m_fd);
    //WSACleanup();
}


int TcpServer::setListen(unsigned short port)
{
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;  // 0 = 0.0.0.0

    printf("%s: bind socket ip:%s, port:%d \n", __func__,inet_ntoa(saddr.sin_addr),port);
    int ret = bind(m_fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        perror("bind");
        return -1;
    }

    ret = listen(m_fd, 128);
    if (ret == -1)
    {
        perror("listen");
        return -1;
    }
    printf("%s: start listen...\n", __func__);

    return ret;
}

TcpSocket* TcpServer::acceptConn(sockaddr_in* addr)
{
    if (addr == NULL)
    {
        return nullptr;
    }

    printf("%s: entry \n", __func__);
    int cfd = -1;
#ifdef MINGW32
    cfd = accept(m_fd, (struct sockaddr*)NULL, NULL);
#else
    socklen_t addrlen = sizeof(struct sockaddr_in);
    cfd = accept(m_fd, (struct sockaddr*)addr, &addrlen);
#endif
    if (cfd == -1)
    {
        printf("%s: cannot create socket client\n", __func__);
        return nullptr;
    }

    printf("%s: new socket client\n", __func__);
    return new TcpSocket(cfd);
}
