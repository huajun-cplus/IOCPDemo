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


#define BUFF_SIZE           (1024*4)                // I/O ����Ļ�������С
#define MAX_POST_ACCEPT     (10)                    // ͬʱͶ�ݵ�Accept����
#define EXIT_CODE           (-1)                    // ���ݸ�Worker�̵߳��˳��ź�
#define DEFAULT_IP          ("127.0.0.1")           // Ĭ��IP��ַ
#define DEFAULT_PORT        (10240)                 // Ĭ�϶˿�


void closeSocket(SOCKET &fd);
void closeHandle(HANDLE &handle);
