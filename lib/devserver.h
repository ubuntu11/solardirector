
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __DEVSERVER_H
#define __DEVSERVER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#endif

typedef struct devserver_config devserver_config_t;

#define DEVSERVER_NAME_LEN 32
#define DEVSERVER_TYPE_LEN 8

struct devserver_io {
	char name[DEVSERVER_NAME_LEN];
	char type[DEVSERVER_TYPE_LEN];
	void *handle;
	int (*open)(void *handle);
	int (*read)(void *handle, void *buf, int buflen);
	int (*write)(void *handle, void *buf, int buflen);
	int (*close)(void *handle);
};
typedef struct devserver_io devserver_io_t;

#define DEVSERVER_MAX_UNITS 8

struct devserver_config {
	devserver_io_t units[DEVSERVER_MAX_UNITS];
	int count;
};

enum DEVSERVER_FUNC {
        DEVSERVER_SUCCESS,
        DEVSERVER_ERROR,
        DEVSERVER_OPEN,
        DEVSERVER_READ,
        DEVSERVER_WRITE,
        DEVSERVER_CLOSE,
};

#ifdef __WIN32
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
#endif

int devserver_send(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, uint16_t data_len);
int devserver_recv(socket_t fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout);
int devserver_request(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len);
int devserver_add_unit(devserver_config_t *, devserver_io_t *);
int devserver(devserver_config_t *,int);

#endif
