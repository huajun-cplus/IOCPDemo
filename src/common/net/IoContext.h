#pragma once
#include "iocpDef.h"


enum class eIoOpType
{
    NONE,           // 用于初始化，无意义
    ACCEPT_POSTED,  // 投递Accept操作
    SEND_POSTED,    // 投递Send操作
    RECV_POSTED,    // 投递Recv操作
};

struct IoContext
{

public:
    IoContext();
    ~IoContext();

    void clear();

    WSAOVERLAPPED       overLapped;        // 每个socket的每一个IO操作都需要一个重叠结构
    SOCKET              socket;            // 此IO操作对应的socket
    WSABUF              wsaBuf;            // 数据缓冲
    eIoOpType           eType;             // IO操作类型
    uint32              u32ConnectID;      // 连接ID
};

