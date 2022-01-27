
#ifndef __SD_SOCKET_H
#define __SD_SOCKET_H

#include <sys/types.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <fcntl.h>

#ifdef __WIN32
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
#endif

#endif
