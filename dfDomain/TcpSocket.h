#ifndef BST_TCP_SOCKET_H
#define BST_TCP_SOCKET_H

#include <string.h>
#include <string>
#ifdef MINGW32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

#define MAGIC_NUMBER 0x55667788
#define SOCK_BUF_SIZE 1024

class TcpSocket
{
public:
    TcpSocket();
    TcpSocket(int fd);
    ~TcpSocket();
    void closeSocket();

    int connectToHost(std::string ip, int port);
    int sendMsg(const char *pBuf, int size);
    int sendMsg_magicNum(const char *pBuf, int size);
    int recvMsg(char *pBuf, int size);
    int getSockFd() { return m_fd; };
    int sendMsg(std::string msg);
    std::string recvMsg();


private:
    int writen(const char *pBuf, int size);
    int readn(char *pBuf, int size);
private:
    int m_fd;
};

#endif  