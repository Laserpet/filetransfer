
#include "TcpSocket.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#define TCP_RETRY_TIME  1

TcpSocket::TcpSocket()
{
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        printf("%s: Failed to create socket, errno %d\n", __func__, errno);
    }
    printf("%s: Create fd: %d", __func__, m_fd);
}

TcpSocket::TcpSocket(int fd)
{
   m_fd = fd; 
}

TcpSocket::~TcpSocket()
{
    if (m_fd > 0) {
        printf("%s: Delete fd: %d\n", __func__, m_fd);
        #ifdef MINGW32
            closesocket(m_fd);
        #else
            close(m_fd);
        #endif
    }
}

void TcpSocket::closeSocket()
{
    printf("%s: closeSocket fd: %d\n", __func__, m_fd);
    if (m_fd > 0) {
        #ifdef MINGW32
            closesocket(m_fd);
        #else
            close(m_fd);
        #endif
        m_fd = -1;
    }
}

int TcpSocket::connectToHost(std::string ip, int port)
{
    int ret = -1; 
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ip.data());

    printf("%s: Connect to %s:%d...\n", __func__, ip.c_str(), port);
    for (int retry = 0; retry < TCP_RETRY_TIME; retry++) {
        ret = connect(m_fd, (struct sockaddr*)&saddr, sizeof(saddr));
        if (ret == -1) {
            printf("%s: Failed to connect %s:%d, retry: %d, errno[%s]\n", __func__, ip.c_str(), port, retry, strerror(errno));
            sleep(1);
        } else {
            printf("%s: Connect to %s:%d success.", __func__, ip.c_str(), port);
            break;
        }
    }
    
    return ret;
}

int TcpSocket::sendMsg(const char *pBuf, int size)
{
    int ret;
    char *pData; 
    int msgHeader; 


    pData = new char[size + sizeof(msgHeader)];
    msgHeader = htonl(size);
    memcpy(pData, &msgHeader, sizeof(msgHeader));
    memcpy(pData + sizeof(msgHeader), pBuf, size);
    ret = writen(pData, size + sizeof(msgHeader));

    delete[] pData;
    return ret;
}

int TcpSocket::sendMsg_magicNum(const char *pBuf, int size)
{
    int ret;
    char *pData; 
    int msgHeader; 
    int magicNumber = MAGIC_NUMBER;
    int dataLen = sizeof(magicNumber) + sizeof(msgHeader) + size;
    
    pData = new char[dataLen];
    memcpy(pData, &magicNumber, sizeof(magicNumber));
    
    msgHeader = htonl(size);
    memcpy(pData + sizeof(magicNumber), &msgHeader, sizeof(msgHeader));
    memcpy(pData + sizeof(magicNumber) + sizeof(msgHeader), pBuf, size);

    ret = writen(pData, dataLen);

    printf("socket send msg with magicNumber = 0x%x\n", magicNumber);

    delete[] pData;
    return ret;
}

int TcpSocket::recvMsg(char *pBuf, int size)
{
    return readn(pBuf, size);
}

int TcpSocket::writen(const char *pBuf, int size)
{
    int leftBytes = size;
    int writtenBytes = 0;

    while (leftBytes > 0) {
        #ifdef MINGW32
        writtenBytes = send(m_fd, pBuf, leftBytes, 0);
        #else
        writtenBytes = write(m_fd, pBuf, leftBytes);
        #endif
        if (writtenBytes <= 0)
            break;
        pBuf += writtenBytes;
        leftBytes -= writtenBytes;
    }

    return writtenBytes;
}

int TcpSocket::readn(char *pBuf, int size)
{
    int leftBytes = size;
    int readBytes = 0;

    while (leftBytes > 0) {
        #ifdef MINGW32
        readBytes = recv(m_fd, pBuf, leftBytes, 0);
        #else
        readBytes = read(m_fd, pBuf, leftBytes);
        #endif
        if (readBytes <= 0)
            break;
        pBuf += readBytes;
        leftBytes -= readBytes;
    }

    return readBytes;
}

/* not used */
int TcpSocket::sendMsg(std::string msg)
{
    //example code
    char* data = new char[msg.size() + 4];
    int bigLen = htonl(msg.size());
    memcpy(data, &bigLen, 4);
    memcpy(data + 4, msg.data(), msg.size());
    // write
    int ret = writen(data, msg.size() + 4);
    delete[]data;
    return ret;
}

std::string TcpSocket::recvMsg()
{
    int len = 0;
    int ret = 0;

    ret = readn((char*)&len, 4);
    if(ret <= 0)
    {
        return std::string();
    }
    len = ntohl(len);

    // malloc buf
    char* buf = new char[len + 1];
    ret = readn(buf, len);
    if (ret != len)
    {
        return std::string();
    }
    buf[len] = '\0';
    std::string retStr(buf);
    delete[]buf;

    return retStr;
}

