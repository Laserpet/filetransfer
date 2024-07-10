#include "domain.h"

ServerNode::ServerNode()
{
    printf("[%s] entry\n",  __func__);
}

ServerNode::~ServerNode(){
    printf("[%s] entry\n",  __func__);

    deinit();
}


int ServerNode::init()
{
    printf("[%s] entry\n",  __func__);
    int         ret = 0;

    PcUi_mTcpSocket = NULL;  //fd
    PcUi_mTcpServer = NULL;  //TCP-instance

    PcUi_mTcpServer = new TcpServer();
    PcUi_mTcpServer->setListen(mPortPcUi);

    Pcui_mSocketAcceptRuning = true;
    PcUi_mAcceptThread = std::thread(std::bind(&ServerNode::PcUi_acceptThread, this));

    printf("[%s] success\n",  __func__);
    return ret;
}

void ServerNode::deinit()
{
    printf("[%s] entry\n",  __func__);
  
    Pcui_mSocketAcceptRuning = false;        
    PcUi_mAcceptThread.join();

    delete PcUi_mTcpServer;
    PcUi_mTcpServer = NULL;

    Pcui_mSocketReceiveRuning = false;
    PcUi_mReceiveThread.join();
}


void ServerNode::PcUi_acceptThread()
{
    sockaddr_in addr;
    TcpSocket* tcpSocketPtr = nullptr;

    printf("Thread[%lu][%s] Create\n", pthread_self(), __func__);

    while (Pcui_mSocketAcceptRuning)
    {
        if (!bExit)
        {
            printf("domainSimulator SocketAcceptThread: Waiting for UI connection\n");

            // 调用 accept 并阻塞，直到有新的连接
            tcpSocketPtr = PcUi_mTcpServer->acceptConn(&addr);
            if (!tcpSocketPtr)
            {
                printf("pcui acceptConn failed\n");
                continue;
            }

            PcUi_mTcpSocket = tcpSocketPtr;
            /* notify connected */
            PcUi_processConnected();
        }
        else
        {
            // 休眠，避免空循环占用 CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    printf("Thread[%lu][%s] Exit\n", pthread_self(), __func__);
}

void ServerNode::PcUi_receiveThread()
{
    int ret = 0;
    int received_magic_number = 0;
    uint32_t msglen = 0;
    uint8_t byte = 0;
    int shift_count = 0;

    printf("Thread[%lu][%s] Create\n", pthread_self(), __func__);

    while (Pcui_mSocketReceiveRuning && !bExit)
    {
        if (PcUi_mTcpSocket == NULL || PcUi_mTcpSocket->getSockFd() < 0) {
            sleep(1);
            continue;
        }

        // 4. 接收数据
        PcUi_receiveData();
    }

    delete PcUi_mTcpSocket;
    PcUi_mTcpSocket = NULL;
    printf("Thread[%lu][%s] Exit\n", pthread_self(), __func__);
}


void ServerNode::PcUi_processConnected()
{
    printf("[%s] entry\n",  __func__);
    if (bExit) {
        Pcui_mSocketReceiveRuning = false;
        PcUi_mReceiveThread.join();
    }

    bExit =  false;
    Pcui_mSocketReceiveRuning = true;
    PcUi_mReceiveThread = std::thread(std::bind(&ServerNode::PcUi_receiveThread, this));
}


int ServerNode::PcUi_receiveData()
{
    int         ret = 0;
    char*       buf = NULL;
    int         len = 0;
    int*        pLen = NULL;
    int*        PcUiCmdType = NULL;
    time_t      timestamps;
    int    received_magic_number = 0;
    int    magicNumber = 0;
    uint8_t     byte = 0;
    int         shift_count = 0;
    
    // 1. 接收 magic number（逐字节接收）
    while (true)
    {
        ret = PcUi_mTcpSocket->recvMsg((char*)&byte, 1);
        if (ret <= 0)
        {
            printf("recvMsg magic number error, errno %d\n", errno);
            return -1;
        }

        // 将接收到的字节按顺序拼接成一个 4 字节的数
        received_magic_number = (received_magic_number << 8) | byte;
        shift_count++;

        // 检查是否已经接收了 4 个字节
        if (shift_count >= 4)
        {
            magicNumber = ntohl(received_magic_number);
            if (magicNumber == MAGIC_NUMBER)
            {
                //printf("%s: df recvMsg magic number: 0x%x)\n", __func__, magicNumber);
                break;
            }
            else
            {
                // 若 magic number 不正确，继续滑动接收
                printf("%s: df: Failed to recvMsg magic number: 0x%x  continue recv)\n", __func__, magicNumber);
                received_magic_number &= 0x00FFFFFF; // 保留低 3 字节
                shift_count = 3;
            }
        }
    }

    // 2. 接收数据长度
    ret = PcUi_mTcpSocket->recvMsg((char*)&len, sizeof(len));
    if (ret <= 0)
    {
        printf("recvMsg hdr error, errno %d\n", errno);
        return -1;
    }
    len = ntohl(len);

    if (!len || len > SOCK_BUF_SIZE)
    {
        printf("recv len is out of range %d\n", len);
        return -1;
    }

    // 3. 接收数据
    buf = (char *)malloc(sizeof(len) + len);
    if (!buf)
    {
        printf("malloc fail, errno %d\n", errno);
        return -1;
    }
    pLen = (int *)buf;

    *pLen = len;
    ret = PcUi_mTcpSocket->recvMsg((char*)buf + sizeof(len), len);
    if (ret <= 0)
    {
        printf("%s: recvMsg dat error, errno %d\n", __func__, errno);
        free(buf);
        return -1;
    }

    // 4. 处理数据
    PcUi_processCmd((char *)buf); // [交互指令数据Len + u32Cmd + u64TimeStamp + strTag / heartBart]

    free(buf);
    return 0;
}


void ServerNode::PcUi_processCmd(char* message)
{
    bool ret = true;
    int* PcUiCmdType = nullptr;
    struct timeval      timev;
    time_t timestamps;
    uint8_t feedBack[2] = {0, domain_mBoardIp};
    Databag db;
    PcUiCmdType = ((int*)message + 1);

    switch(*PcUiCmdType)
    {
        case MSGREQ_PC_TO_DOMAIN_HEARTBEAT:
            printf("Rcv Cmd: MSGREQ_PC_TO_DOMAIN_HEARTBEAT\n");
            {
                MsgHeartBeart_info_t heartBeatInfo;
                gettimeofday(&timev, NULL);
                heartBeatInfo.boardIp = domain_mBoardIp;
                heartBeatInfo.timeStamp = (uint64_t)timev.tv_sec * 1000000 + timev.tv_usec;
                heartBeatInfo.rspCnt = *((uint32_t*)(PcUiCmdType + 1));

                printf("Rsp Cmd: MSGRSP_DOMAIN_TO_PC_HEARTBEAT rspCnt[%d]\n", heartBeatInfo.rspCnt);
                PcUi_Repeson(MSGRSP_DOMAIN_TO_PC_HEARTBEAT, (char *)&heartBeatInfo, sizeof(heartBeatInfo));
            }
            break;
        case MSGREQ_PC_TO_DOMAIN_TRIG_TAG:
            printf("Rcv Cmd: MSGREQ_PC_TO_DOMAIN_TRIG_TAG\n");
            {
                int* len = (int*)message;
                timestamps = *((uint64_t*)(PcUiCmdType + 1));
                int* PcUiTagData = (PcUiCmdType + 3);

                int tagLen = (*len - 4 - 8);  //交互指令数据Len - u32Cmd - u64TimeStamp 
                char tagInfo[128] = {0};
                
                memcpy(tagInfo, PcUiTagData, tagLen);

                printf("Rsp Cmd: recvMsg PcUiCmdType[%d], timestamps[%long long unsigned int], TAG_INFO: %s\n", *PcUiCmdType, timestamps, tagInfo);

                feedBack[0] = (uint8_t)ret;
                PcUi_Repeson(MSGRSP_DOMAIN_TO_PC_TRIG_TAG, (char *)feedBack, sizeof(feedBack));
                // 发送S2给 Server_socket程序
                db.Send_msg_databag(2);

            }
            break;
        case MSGREQ_PC_TO_DOMAIN_EXIT:
            printf("Rcv Cmd: >>>>>>>>>   MSGREQ_PC_TO_DOMAIN_EXIT  >>>>>>>>>  \n");
            {
                feedBack[0] = (uint8_t)ret;
                PcUi_Repeson(MSGRSP_DOMAIN_TO_PC_EXIT, (char *)feedBack, sizeof(feedBack));
                printf("Rsp Cmd: >>>>>>>>>   MSGRSP_DOMAIN_TO_PC_EXIT  >>>>>>>>>  \n");
                bExit = true;             
            }
            break;
        default:
            printf("PcUi_processCmd PC_TO_DOMAIN msg_type undefine: %d\n", *PcUiCmdType);
            break; 
    }
}

void ServerNode::PcUi_Repeson(int PcUi_MsgType, char* PcUi_IndInfo, const int DataLen)
{
    char    PcUi_dumpInfo[DataLen + 4] = {0};
    memcpy(PcUi_dumpInfo, &PcUi_MsgType, 4);
    memcpy(PcUi_dumpInfo + 4, PcUi_IndInfo, DataLen);
    if(PcUi_mTcpSocket)
    {
        PcUi_mTcpSocket->sendMsg_magicNum(PcUi_dumpInfo, DataLen + 4);
    }
}

int Databag::Send_msg_databag(int cmd){
    std::string ip = "127.0.0.1";
    int port = 6668;
    std::string message = "S2";
    if(cmd == 2){
        printf("[+] Sending Msg S2 to databag server_socket\n");
        if (socket_msg(ip, port, message)) {
            printf("S2 Message sent successfully\n");
        }else {
                    printf("Failed to send message\n");
        }
            sleep(1);
    }
    return 0;
}

bool Databag::socket_msg(const std::string& ip, int port, const std::string& message) {
    int sockfd;
    struct sockaddr_in server_addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error creating socket\n");
        return false;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error connecting to server\n");
        close(sockfd);
        return false;
    }

    // 发送消息
    if (send(sockfd, message.c_str(), message.length(), 0) < 0) {
        printf("Error sending message\n");
        close(sockfd);
        return false;
    }
    printf("Msg Sent success\n");
    // 关闭套接字
    close(sockfd);

    return true;
}