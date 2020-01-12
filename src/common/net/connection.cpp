#include "connection.h"

#include "iocpDef.h"


CConnection::CConnection(SOCKET _fd, const SOCKADDR_IN &_addr)
    : socket(_fd)
    , m_addr(_addr){
}

CConnection::~CConnection() {
    closeSocket(socket);
    ZeroMemory(&m_addr, sizeof(m_addr));
}

SOCKET CConnection::getSocket() const {
    return socket;
}

const SOCKADDR_IN & CConnection::getAddr() const {
    return m_addr;
}
