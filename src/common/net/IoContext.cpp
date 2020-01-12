#include "IOContext.h"


IoContext::IoContext() {
    ZeroMemory(&overLapped, sizeof(overLapped));
    socket = INVALID_SOCKET;

    wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFF_SIZE);
    wsaBuf.len = BUFF_SIZE;

    eType = eIoOpType::NONE;
    u32ConnectID = 0;
}

IoContext::~IoContext() {
    socket = INVALID_SOCKET;

    if (wsaBuf.buf != nullptr) {
        HeapFree(::GetProcessHeap(), 0, wsaBuf.buf);
    }
}

void IoContext::clear() {
    ZeroMemory(&overLapped, sizeof(overLapped));

    if (wsaBuf.buf != nullptr) {
        ZeroMemory(wsaBuf.buf, BUFF_SIZE);
    } else {
        wsaBuf.buf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFF_SIZE);
    }

    eType = eIoOpType::NONE;
    u32ConnectID = 0;
}
