#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include<WinSock2.h>
#include <MSWSock.h>

#pragma comment(lib,"ws2_32.lib")

typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;


#define BUFF_SIZE           (1024*4)                // I/O 请求的缓冲区大小
#define MAX_POST_ACCEPT     (10)                    // 同时投递的Accept数量
#define EXIT_CODE           (-1)                    // 传递给Worker线程的退出信号
#define DEFAULT_IP          ("127.0.0.1")           // 默认IP地址
#define DEFAULT_PORT        (10240)                 // 默认端口


void closeSocket(SOCKET &fd);
void closeHandle(HANDLE &handle);
