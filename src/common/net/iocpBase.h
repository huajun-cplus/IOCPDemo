#pragma once

#include <vector>
#include <string>

#include "iocpDef.h"

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

private:
    bool initIocp();
    bool initListenSocket();
    bool initExFun();
    bool initWorkerThread();
    bool initPostAccept();

    bool postAccept(IoContext &ioContext);

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
