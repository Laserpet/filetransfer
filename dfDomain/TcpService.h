/**
 * Copyright(c): Black Sesame Technologies(Shanghai)Co.,Ltd. All rights reserved.
 * Filename:
 * Author: Benson.wang
 * Date: 2022年 01月 11日
 **/

#ifndef BST_TCP_SERVICE_H
#define BST_TCP_SERVICE_H

#include "TcpSocket.h"


class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    int setListen(unsigned short port);
    TcpSocket* acceptConn(struct sockaddr_in* addr = nullptr);
    int getSockFd() { return m_fd; };

private:
    int m_fd;
};

#endif