#include "iocpBase.h"

#include <iostream>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <assert.h>

#include "ioContext.h"
#include "connection.h"


CIocpBase::CIocpBase()
    : m_iocp(INVALID_HANDLE_VALUE)
    , m_u16ListenPort(0)
    , m_listenSocket(INVALID_SOCKET)
    , m_fnAcceptEx(nullptr)
    , m_fnGetAcceptExSockAddrs(nullptr) {
    m_stopEvent = CreateEvent(nullptr, true, false, nullptr);
}

CIocpBase::~CIocpBase() {
    clear();
}

bool CIocpBase::init(const std::string &strIp, uint16 u16Port) {
    this->m_strListenIp = strIp;
    this->m_u16ListenPort = u16Port;

    // 初始化完成端口
    if (!initIocp()) {
        return false;
    }

    // 初始化监听文件描述
    if (!initListenSocket()) {
        return false;
    }

    // 初始化拓展方法
    if (!initExFun()) {
        return false;
    }

    // 初始化工作线程
    if (!initWorkerThread()) {
        return false;
    }

    // 初始化投递accept
    if (!initPostAccept()) {
        return false;
    }

    return true;
}

void CIocpBase::clear() {
    // IoContext目前暂未处理结束时的回收

    // 激活关闭事件
    SetEvent(m_stopEvent);

    // 通知所有完成端口退出
    auto szThreadNum = m_vecWorkerThreads.size();
    for (decltype(szThreadNum) i = 0; i < szThreadNum; i++)
    {
        PostQueuedCompletionStatus(m_iocp, 0, static_cast<ULONG_PTR>(EXIT_CODE), nullptr);
    }

    // 等待所有工作线程退出
    HANDLE * pThreads = new HANDLE[szThreadNum];
    for (decltype(szThreadNum) i = 0; i < szThreadNum; i++)
    {
        pThreads[i] = m_vecWorkerThreads[i];
    }

    WaitForMultipleObjects(static_cast<DWORD>(szThreadNum), pThreads, TRUE, INFINITE);
    delete [] pThreads;
    pThreads = nullptr;

    // 释放工作者线程句柄指针
    for (int i = 0; i< szThreadNum; i++)
    {
        closeHandle(m_vecWorkerThreads[i]);
    }

    closeSocket(m_listenSocket);
    closeHandle(m_stopEvent);
    closeHandle(m_iocp);

    m_vecWorkerThreads.clear();
    m_strListenIp.clear();
    m_u16ListenPort = 0;

    m_fnAcceptEx = nullptr;
    m_fnGetAcceptExSockAddrs = nullptr;
}

bool CIocpBase::initIocp() {
    this->m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    return INVALID_HANDLE_VALUE != this->m_iocp;
}

bool CIocpBase::initListenSocket() {
    // 生成用于监听的Socket
    this->m_listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == this->m_listenSocket) {
        return false;
    }

    // 将socket绑定到完成端口中
    if (INVALID_HANDLE_VALUE == CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(this->m_listenSocket)
            , this->m_iocp
            , static_cast<ULONG_PTR>(this->m_listenSocket)
            , 0)) {
        return false;
    }

    //服务器地址信息，用于绑定socket
    sockaddr_in serverAddr;

    // 填充地址信息
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

    // 绑定地址和端口
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

    // 开始监听
    if (SOCKET_ERROR == listen(this->m_listenSocket, SOMAXCONN)) {
        assert(false);
        return false;
    }

    return true;
}

bool CIocpBase::initExFun() {
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    GUID guidGetAcceptSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

    // 提取扩展函数指针
    DWORD dwBytes = 0;
    if (SOCKET_ERROR == WSAIoctl(
            this->m_listenSocket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guidAcceptEx,
            sizeof(guidAcceptEx),
            &this->m_fnAcceptEx,
            sizeof(this->m_fnAcceptEx),
            &dwBytes,
            nullptr,
            nullptr)) {
        assert(false);
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
            nullptr,
            nullptr)) {
        assert(false);
        return false;
    }

    return true;
}

bool CIocpBase::initWorkerThread() {
    // 获取处理器数量
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    int32 n32WorkerThreadNum = static_cast<int32>(si.dwNumberOfProcessors)
        * static_cast<int32>(eConstDef::PRE_PROCESSOR_THREAD_NUM);

    for (int i = 0; i < n32WorkerThreadNum; i++)
    {
        m_vecWorkerThreads.emplace_back(CreateThread(0, 0, workerThreadProc, reinterpret_cast<void *>(this), 0, 0));
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
        pIoContext->socket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

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
    if (INVALID_SOCKET == ioContext.socket) {
        assert(false);
        return false;
    }

    ioContext.eType = eIoOpType::ACCEPT_POSTED;

    // 将接收缓冲置为0,令AcceptEx直接返回
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

bool CIocpBase::postRecv(IoContext &ioContext) {
    if (INVALID_SOCKET == ioContext.socket) {
        assert(false);
        return false;
    }

    ioContext.eType = eIoOpType::RECV_POSTED;
    DWORD dwFlags = 0, dwBytes = 0;

    int nBytesRecv = WSARecv(
        ioContext.socket
        , &ioContext.wsaBuf
        , 1
        , &dwBytes
        , &dwFlags
        , &ioContext.overLapped
        , nullptr);

    // 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
    if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError())) {
        return false;
    }

    return true;
}

bool CIocpBase::postSend(IoContext &ioContext) {
    if (INVALID_SOCKET == ioContext.socket) {
        assert(false);
        return false;
    }

    ioContext.eType = eIoOpType::SEND_POSTED;
    DWORD dwFlags = 0, dwBytes = 0;

    if (::WSASend(ioContext.socket, &ioContext.wsaBuf, 1, &dwBytes, dwFlags, &ioContext.overLapped, nullptr) != NO_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            return false;
        }
    }
    return true;
}

void CIocpBase::doAccpet(CConnection &connection, IoContext &ioContext) {
    sockaddr_in *pClientAddr = nullptr;
    sockaddr_in *pLocalAddr = nullptr;
    int nClientAddrLen, nLocalAddrLen;
    nClientAddrLen = nLocalAddrLen = sizeof(sockaddr_in);

    // 1. 获取地址信息 （GetAcceptExSockAddrs函数不仅可以获取地址信息，还可以顺便取出第一组数据）
    m_fnGetAcceptExSockAddrs(
        ioContext.wsaBuf.buf
        , 0
        , nLocalAddrLen
        , nClientAddrLen
        , reinterpret_cast<sockaddr **>(&pLocalAddr)
        , &nLocalAddrLen
        , reinterpret_cast<sockaddr **>(&pClientAddr)
        , &nClientAddrLen);

    // 2. 为新连接建立一个CConnection
    CConnection *pNewConnection = new CConnection(ioContext.socket, *pClientAddr);

    // 3. 将listenSocketContext的IOContext 重置后继续投递AcceptEx
    ioContext.clear();
    if (!this->postAccept(ioContext)) {
        delete &ioContext;
    }

    // 4. 将新socket和完成端口绑定
    if (nullptr != pNewConnection) {
        return;
    }

    if (nullptr == CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(pNewConnection->getSocket())
        , this->m_iocp
        , reinterpret_cast<ULONG_PTR>(pNewConnection)
        , 0)) {
        DWORD dwErr = WSAGetLastError();
        if (dwErr != ERROR_INVALID_PARAMETER) {
            char clientIP[24] = { 0 };
            auto pRes = inet_ntop(AF_INET, &pNewConnection->getAddr().sin_addr, clientIP, sizeof(clientIP));

            std::cerr
                << "client addr("
                << clientIP
                <<":"
                << ntohs(pNewConnection->getAddr().sin_port)
                <<") create io completion port error, ec: "
                << WSAGetLastError()
                << std::endl;

            assert(false);
            doClose(*pNewConnection);
            return;
        }
    }

    // 并设置tcp_keepalive
    tcp_keepalive alive_in;
    tcp_keepalive alive_out;
    alive_in.onoff = TRUE;
    alive_in.keepalivetime = KEEP_ALIVE_TIME;
    alive_in.keepaliveinterval = KEEP_ALIVE_INTERVAL;
    unsigned long ulBytesReturn = 0;
    if (SOCKET_ERROR == WSAIoctl(
        pNewConnection->getSocket()
        , SIO_KEEPALIVE_VALS
        , &alive_in
        , sizeof(alive_in)
        , &alive_out
        , sizeof(alive_out)
        , &ulBytesReturn
        , nullptr
        , nullptr)) {
        std::cerr
            <<"WSAIoctl failed, ec: "
            << WSAGetLastError()
            << std::endl;
    }

    // 回调
    onConnectionEstablished(*pNewConnection);

    // 5. 建立recv操作所需的ioContext，在新连接的socket上投递recv请求
    IoContext *pNewIoContext = new IoContext;
    if (nullptr == pNewIoContext) {
        this->doClose(*pNewConnection);
        return;
    }

    pNewIoContext->socket = pNewConnection->getSocket();

    // 投递recv请求
    if (!this->postRecv(*pNewIoContext)) {
        this->doClose(*pNewConnection);

        delete pNewIoContext;
    }
}

void CIocpBase::doRecv(CConnection &connection, IoContext &ioContext) {
    this->onRecvCompleted(connection, ioContext);

    ioContext.clear();

    if (!this->postRecv(ioContext)) {
        this->doClose(connection);
    }
}

void CIocpBase::doSend(CConnection &connection, IoContext &ioContext) {
    this->onSendCompleted(connection, ioContext);

    delete &ioContext;
}

void CIocpBase::doClose(CConnection &connection) {
    this->onConnectionClosed(connection);

    delete &connection;
}

DWORD __stdcall CIocpBase::workerThreadProc(LPVOID pParam){
    if (nullptr == pParam) {
        return -1;
    }

    CIocpBase *pIocpBase = reinterpret_cast<CIocpBase*>(pParam);
    OVERLAPPED *pOverLapped = nullptr;
    CConnection *pConnection = nullptr;
    IoContext *pIoContext = nullptr;
    DWORD dwBytes = 0;

    while (WAIT_OBJECT_0 != WaitForSingleObject(pIocpBase->m_stopEvent, 0)) {
        ULONG_PTR lpCompletionKey;
        bool bRet = GetQueuedCompletionStatus(pIocpBase->m_iocp, &dwBytes, &lpCompletionKey, &pOverLapped, INFINITE);

        // 读取传入的参数
        pIoContext = CONTAINING_RECORD(pOverLapped, IoContext, overLapped);

        // 收到退出标志
        if (static_cast<ULONG_PTR>(EXIT_CODE) == lpCompletionKey) {
            break;
        }

        pConnection = reinterpret_cast<CConnection *>(lpCompletionKey);

        if (nullptr == pIoContext
            || nullptr == pConnection) {
            assert(false);
            continue;
        }

        if (bRet) {
            // 如果收发数据为0 即断开了
            if (0 == dwBytes
                &&(eIoOpType::SEND_POSTED == pIoContext->eType
                    || eIoOpType::RECV_POSTED == pIoContext->eType)) {
                pIocpBase->doClose(*pConnection);

                delete pIoContext;
            }

            // do完后 连接可能被处理了 后面不要再使用了
            switch (pIoContext->eType) {
            case eIoOpType::ACCEPT_POSTED:
                {
                    pIocpBase->doAccpet(*pConnection, *pIoContext);
                }
                break;

            case eIoOpType::SEND_POSTED:
                {
                    pIocpBase->doSend(*pConnection, *pIoContext);
                }
                break;

            case eIoOpType::RECV_POSTED:
                {
                    pIocpBase->doRecv(*pConnection, *pIoContext);
                }
                break;

            default:
                {
                    std::cerr
                        << "io type error: type: "
                        << static_cast<int32>(pIoContext->eType)
                        << std::endl;

                    pIocpBase->doClose(*pConnection);

                    delete pIoContext;
                }
                break;
            }
        } else {
            auto eEc = GetLastError();
            if (ERROR_NETNAME_DELETED != eEc) {
                std::cerr
                    << "get queued completion status faild: ec: "
                    << GetLastError()
                    << std::endl;

                assert(ERROR_NETNAME_DELETED == GetLastError());
            }

            pIocpBase->doClose(*pConnection);

            delete pIoContext;
        }
    }

    return 0;
}
