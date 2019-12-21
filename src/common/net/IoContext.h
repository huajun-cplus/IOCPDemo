#pragma once
#include "iocpDef.h"


enum class eIoOpType
{
    NONE,           // ���ڳ�ʼ����������
    ACCEPT_POSTED,  // Ͷ��Accept����
    SEND_POSTED,    // Ͷ��Send����
    RECV_POSTED,    // Ͷ��Recv����
};

struct IoContext
{

public:
    IoContext();
    ~IoContext();

    void clear();

    WSAOVERLAPPED       overLapped;        // ÿ��socket��ÿһ��IO��������Ҫһ���ص��ṹ
    SOCKET              socket;            // ��IO������Ӧ��socket
    WSABUF              wsaBuf;            // ���ݻ���
    eIoOpType           eType;             // IO��������
    uint32              u32ConnectID;      // ����ID
};

