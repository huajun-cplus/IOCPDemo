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
        PRE_PROCESSOR_THREAD_NUM = 2,   // 每个处理器上的线程数
    };

public:
    CIocpBase();
    virtual ~CIocpBase();

    bool init(const std::string &strIp, uint16 u16Port);
    void clear();

public:
    // 新连接
    virtual void onConnectionEstablished(CConnection &connection) = 0;
    // 连接关闭
    virtual void onConnectionClosed(CConnection &connection) = 0;
    // 读操作完成
    virtual void onRecvCompleted(CConnection &connection, IoContext &ioContext) = 0;
    // 写操作完成
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

    static DWORD WINAPI workerThreadProc(LPVOID pParam); // 工作线程函数

private:
    HANDLE                      m_stopEvent;                // 通知线程退出的事件
    HANDLE                      m_iocp;                     // 完成端口
    std::vector<HANDLE>         m_vecWorkerThreads;         // 工作者线程的句柄
    std::string                 m_strListenIp;              // 监听IP
    uint16                      m_u16ListenPort;            // 监听端口
    SOCKET                      m_listenSocket;             // 监听文件描述符

    LPFN_ACCEPTEX               m_fnAcceptEx;               // AcceptEx函数指针
    LPFN_GETACCEPTEXSOCKADDRS   m_fnGetAcceptExSockAddrs;   // GetAcceptExSockAddrs函数指针
};
