#include "IocpDef.h"


void closeSocket(SOCKET &fd) {
    if (INVALID_SOCKET == fd) {
        return;
    }

    closesocket(fd);
    fd = INVALID_SOCKET;
}

void closeHandle(HANDLE &handle) {
    if (INVALID_HANDLE_VALUE == handle) {
        return;
    }

    CloseHandle(handle);
    handle = INVALID_HANDLE_VALUE;
}
