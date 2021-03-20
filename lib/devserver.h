
#ifndef __DEVSERVER_H
#define __DEVSERVER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct devserver_config devserver_config_t;

#define DEVSERVER_NAME_LEN 32
#define DEVSERVER_DEVICE_LEN 64

struct devserver_io {
	char name[DEVSERVER_NAME_LEN+1];
	char device[DEVSERVER_DEVICE_LEN+1];
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

int devserver_send(int fd, uint8_t opcode, uint8_t unit, void *data, int datasz);
int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout);
int devserver_request(int fd, uint8_t opcode, uint8_t unit, void *data, int len);
int devserver_reply(int fd, uint8_t status, uint8_t unit ,uint8_t *data, int len);
int devserver_error(int fd, uint8_t status);
int devserver_add_unit(devserver_config_t *, devserver_io_t *);
int devserver(devserver_config_t *,int);

#if 0
#define devserver_getshort(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_putshort(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_put16(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get32(p) (uint16_t)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))
#define devserver_put32(p,v) *(p) = ((int)(v) & 0xFF); *((p)+1) = ((int)(v) >> 8) & 0xFF; *((p)+2) = ((int)(v) >> 16) & 0xFF; *((p)+3) = ((int)(v) >> 24) & 0xFF
#endif

#endif
