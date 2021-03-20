
#ifndef __SOLARD_MODTYPE_H
#define __SOLARD_MODTYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"
#include "debug.h"

typedef int (*solard_transport_open_t)(void *);
typedef int (*solard_transport_read_t)(void *,...);
typedef int (*solard_transport_write_t)(void *,...);
typedef int (*solard_transport_close_t)(void *);
typedef int (*solard_transport_control_t)(void *,...);

struct solard_transport {
	int type;
	char *name;
	unsigned short capabilities;
	int (*init)(void *);
	void *(*new)(void *,...);
	solard_transport_open_t open;
	solard_transport_read_t read;
	solard_transport_write_t write;
	solard_transport_close_t close;
	int (*free)(void *);
	int (*shutdown)(void *);
	solard_transport_control_t control;
	solard_transport_control_t config;
};
typedef struct solard_transport solard_transport_t;

#define SOLARD_MODTYPE_CAPABILITY_CONTROL 0x01
#define SOLARD_MODTYPE_CAPABILITY_CONFIG	0x02

solard_transport_t *load_transport(list, char *name, int type);

enum SOLARD_TRANSPORT {
        SOLARD_MODTYPE_INVERTER,
        SOLARD_MODTYPE_CELLMON,
        SOLARD_MODTYPE_TRANSPORT,
        SOLARD_MODTYPE_DB,
};

#define SOLARD_TARGET_LEN 32

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif 

#endif /* __SOLARD_MODTYPE_H */
