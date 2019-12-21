#include "iocpBase.h"

#include <iostream>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <assert.h>

#include "ioContext.h"


CIocpBase::CIocpBase()
    : m_iocp(INVALID_HANDLE_VALUE)
    , m_u16ListenPort(0)
    , m_listenSocket(INVALID_SOCKET)
    , m_fnAcceptEx(nullptr)
    , m_fnGetAcceptExSockAddrs(nullptr) {
    m_stopEvent = CreateEvent(nullptr, true, false, nullptr);
}

CIocpBase::~CIocpBase() {

}

bool CIocpBase::init(const std::string &strIp, uint16 u16Port) {
    this->m_strListenIp = strIp;
    this->m_u16ListenPort = u16Port;

    // ��ʼ����ɶ˿�

    // ��ʼ�������ļ�����

    // ��ʼ����չ����

    // ��ʼ�������߳�

    return true;
}

void CIocpBase::clear() {
}

bool CIocpBase::initIocp() {
    this->m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return INVALID_HANDLE_VALUE != this->m_iocp;
}

bool CIocpBase::initListenSocket() {
    // �������ڼ�����Socket
    this->m_listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == this->m_listenSocket) {
        return false;
    }

    // ��socket�󶨵���ɶ˿���
    if (INVALID_HANDLE_VALUE == CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(this->m_listenSocket)
            , this->m_iocp
            , static_cast<ULONG_PTR>(this->m_listenSocket)
            , 0)) {
        return false;
    }

    //��������ַ��Ϣ�����ڰ�socket
    sockaddr_in serverAddr;

    // ����ַ��Ϣ
    ZeroMemory((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->m_u16ListenPort);

    int nRes = inet_pton(AF_INET, this->m_strListenIp.c_str(), &serverAddr.sin_addr);
    if (SOCKET_ERROR == nRes) {
        std::cerr
            << "ip("
            << this->m_strListenIp
            <<") convert error, ec: "
            << WSAGetLastError()
            << std::endl;
        assert(false);
        return false;
    }

    // �󶨵�ַ�Ͷ˿�
    if (SOCKET_ERROR == bind(
        this->m_listenSocket
        , reinterpret_cast<sockaddr *>(&serverAddr)
        , sizeof(serverAddr))) {
        std::cerr
            << "addr("
            << this->m_strListenIp
            <<":"
            << this->m_u16ListenPort
            <<") bind error, ec: "
            << WSAGetLastError()
            << std::endl;
        assert(false);
        return false;
    }

    // ��ʼ����
    if (SOCKET_ERROR == listen(this->m_listenSocket, SOMAXCONN))
    {
        return false;
    }

    return true;
}

bool CIocpBase::initExFun() {
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    GUID guidGetAcceptSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

    // ��ȡ��չ����ָ��
    DWORD dwBytes = 0;
    if (SOCKET_ERROR == WSAIoctl(
            this->m_listenSocket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guidAcceptEx,
            sizeof(guidAcceptEx),
            &this->m_fnAcceptEx,
            sizeof(this->m_fnAcceptEx),
            &dwBytes,
            NULL,
            NULL)) {
        return false;
    }

    if (SOCKET_ERROR == WSAIoctl(
            this->m_listenSocket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guidGetAcceptSockAddrs,
            sizeof(guidGetAcceptSockAddrs),
            &this->m_fnGetAcceptExSockAddrs,
            sizeof(this->m_fnGetAcceptExSockAddrs),
            &dwBytes,
            NULL,
            NULL)) {
        return false;
    }

    return true;
}

bool CIocpBase::initWorkerThread() {
    // ��ȡ����������
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    int32 n32WorkerThreadNum = static_cast<int32>(si.dwNumberOfProcessors)
        * static_cast<int32>(eConstDef::PRE_PROCESSOR_THREAD_NUM);

    for (int i = 0; i < n32WorkerThreadNum; i++)
    {
        m_vecWorkerThreads.emplace_back(CreateThread(0, 0, workerThreadProc, (void *)this, 0, 0));
    }

    return true;
}

bool CIocpBase::initPostAccept() {
    auto szThreadNum = this->m_vecWorkerThreads.size();
    for (size_t i = 0; i < szThreadNum; i++)
    {
        IoContext *pIoContext = new(std::nothrow) IoContext;
        if (nullptr == pIoContext) {
            return false;
        }

        pIoContext->eType = eIoOpType::ACCEPT_POSTED;
        pIoContext->socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (false == this->postAccept(*pIoContext))
        {
            closeSocket(pIoContext->socket);
            delete pIoContext;
            return false;
        }
    }

    return true;
}

bool CIocpBase::postAccept(IoContext &ioContext) {
    if (eIoOpType::ACCEPT_POSTED == ioContext.eType) {
        assert(false);
        return false;
    }

    if (INVALID_SOCKET == ioContext.socket) {
        assert(false);
        return false;
    }

    // �����ջ�����Ϊ0,��AcceptExֱ�ӷ���,��ֹ�ܾ����񹥻�
    DWORD dwBytes = 0;
    if (false == this->m_fnAcceptEx(
        this->m_listenSocket
        , ioContext.socket
        , ioContext.wsaBuf.buf
        , 0
        , sizeof(sockaddr_in) + 16
        , sizeof(sockaddr_in) + 16
        , &dwBytes
        , &ioContext.overLapped)) {
        if (WSA_IO_PENDING != WSAGetLastError()){
            assert(false);
            return false;
        }
    }

    return true;
}

DWORD __stdcall CIocpBase::workerThreadProc(LPVOID pParam) {
    return 0;
}
