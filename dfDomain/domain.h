#ifndef BST_SERVER_NODE_H
#define BST_SERVER_NODE_H

#include <thread>
#include <sys/time.h>
#include <functional>
#include "TcpService.h"

enum MsgType_e {
    MSGREQ_PC_TO_DOMAIN_BASE = 600,
    MSGREQ_PC_TO_DOMAIN_HEARTBEAT,
    MSGREQ_PC_TO_DOMAIN_TRIG_TAG,      /*触发储存*/
    MSGREQ_PC_TO_DOMAIN_EXIT,

    MSGRSP_DOMAIN_TO_PC_BASE = 700,
    MSGRSP_DOMAIN_TO_PC_HEARTBEAT,
    MSGRSP_DOMAIN_TO_PC_TRIG_TAG,
    MSGRSP_DOMAIN_TO_PC_EXIT,
    MSGRSP_DOMAIN_TO_PC_MAX
};


typedef struct MsgHeartBeart_info
{
    uint32_t    boardIp;     // 可不填写：ip地址的最后字段              // 4 byte
    uint32_t    rspCnt;      // 心跳计次：将接收到的心跳计次发送回去    // 4 byte
    uint64_t    timeStamp;   // 可不填写：域控时间戳                    // 8 byte
}MsgHeartBeart_info_t;


class ServerNode{
public:
    ServerNode();
    virtual ~ServerNode ();
    int init();
    void deinit();

private:
    void PcUi_acceptThread();
    void PcUi_receiveThread();

    void PcUi_processConnected();
    int  PcUi_receiveData();
    void PcUi_processCmd(char* message);
    void PcUi_Repeson(int PcUi_MsgType, char* PcUi_IndInfo, const int DataLen);
    
private:

    uint8_t domain_mBoardIp = 66;
    unsigned short mPortPcUi = 6666;


    std::string PcUi_mBoardName = "A";


    TcpSocket* PcUi_mTcpSocket = nullptr;
    TcpServer* PcUi_mTcpServer = nullptr;


    std::thread PcUi_mAcceptThread;
    std::thread PcUi_mReceiveThread;
    bool Pcui_mSocketAcceptRuning = false;
    bool Pcui_mSocketReceiveRuning = false;
    bool bExit = false;
};

class Databag{
public:
    int Send_msg_databag(int cmd);
    bool socket_msg(const std::string& ip, int port, const std::string& message);
};

#endif //BST_SERVER_NODE_H
