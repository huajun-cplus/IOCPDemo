#pragma once

#include <vector>
#include <string>

#include "iocpDef.h"

class CConnection;
struct IoContext;

class CIocpBase
{
    enum class eConstDef
    {
        PRE_PROCESSOR_THREAD_NUM = 2,   // ÿ���������ϵ��߳���
    };

public:
    CIocpBase();
    virtual ~CIocpBase();

    bool init(const std::string &strIp, uint16 u16Port);
    void clear();

public:
    // ������
    virtual void onConnectionEstablished(CConnection &connection) = 0;
    // ���ӹر�
    virtual void onConnectionClosed(CConnection &connection) = 0;
    // ���������
    virtual void onRecvCompleted(CConnection &connection, IoContext &ioContext) = 0;
    // д�������
    virtual void onSendCompleted(CConnection &connection, IoContext &ioContext) = 0;

private:
    bool initIocp();
    bool initListenSocket();
    bool initExFun();
    bool initWorkerThread();
    bool initPostAccept();

    bool postAccept(IoContext &ioContext);
    bool postRecv(IoContext &ioContext);
    bool postSend(IoContext &ioContext);

    void doAccpet(CConnection &connection, IoContext &ioContext);
    void doRecv(CConnection &connection, IoContext &ioContext);
    void doSend(CConnection &connection, IoContext &ioContext);
    void doClose(CConnection &connection);

    static DWORD WINAPI workerThreadProc(LPVOID pParam); // �����̺߳���

private:
    HANDLE                      m_stopEvent;                // ֪ͨ�߳��˳����¼�
    HANDLE                      m_iocp;                     // ��ɶ˿�
    std::vector<HANDLE>         m_vecWorkerThreads;         // �������̵߳ľ��
    std::string                 m_strListenIp;              // ����IP
    uint16                      m_u16ListenPort;            // �����˿�
    SOCKET                      m_listenSocket;             // �����ļ�������

    LPFN_ACCEPTEX               m_fnAcceptEx;               // AcceptEx����ָ��
    LPFN_GETACCEPTEXSOCKADDRS   m_fnGetAcceptExSockAddrs;   // GetAcceptExSockAddrs����ָ��
};
