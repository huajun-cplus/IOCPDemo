#pragma once

#include <WinSock2.h>


class CConnection
{
public:
    explicit CConnection(SOCKET socket, const SOCKADDR_IN &&m_addr);
    ~CConnection();

    SOCKET getSocket() const;
    const SOCKADDR_IN & getAddr() const;

private:
    SOCKET          socket;         // 连接的socket
    SOCKADDR_IN     m_addr;       // 连接的远程地址
};

