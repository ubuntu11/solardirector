
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SMANET_INTERNAL_H
#define __SMANET_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "types.h"
#include "smanet.h"
#include "debug.h"

struct smanet_packet {
	uint16_t src;
	uint16_t dest;
	uint8_t control;
	unsigned group: 1;
	unsigned response: 1;
	unsigned block: 1;
	uint8_t count;
	uint8_t command;
	uint8_t *data;
	int datasz;
	int dataidx;
};
typedef struct smanet_packet smanet_packet_t;

/* Commands */
#define CMD_GET_NET		0x01	/* Request for network configuration */
#define CMD_SEARCH_DEV		0x02	/* Find Devices */
#define CMD_CFG_NETADR		0x03	/* Assign network address */
#define CMD_SET_GRPADR		0x04	/* Assign group address */
#define CMD_DEL_GRPADR		0x05	/* Delete group address */
#define CMD_GET_NET_START	0x06	/* Start of configuration */

#define CMD_GET_CINFO		0x09	/* Request for channel information */
#define CMD_SYN_ONLINE		0x0A	/* Synchronize online data */
#define CMD_GET_DATA		0x0B	/* Data request */
#define CMD_SET_DATA		0x0C	/* Transmit data */
#define CMD_GET_SINFO		0x0D	/* Query storge config */
#define CMD_SET_MPARA		0x0F	/* Parameterize data acquisition */

#define CMD_GET_MTIME		0x14	/* Read storage intervals */
#define CMD_SET_MTIME		0x15	/* Set storage intervals */

#define CMD_GET_BINFO		0x1E	/* Request binary range information */
#define CMD_GET_BIN		0x1F	/* Binary data request */
#define CMD_SET_BIN		0x20	/* Send binary data */

#define CMD_TNR_VERIFY		0x32	/* Verify participant number */
#define CMD_VAR_VALUE		0x33	/* Request system variables */
#define CMD_VAR_FIND		0x34	/* Owner of a variable */
#define CMD_VAR_STATUS_OUT	0x35	/* Allocation variable - channel */
#define CMD_VAR_DEFINE_OUT	0x36	/* Allocate variable - channel */
#define CMD_VAR_STATUS_IN	0x37	/* Allocation input parameter - status */
#define CMD_VAR_DEFINE_IN	0x38	/* Allocate input parameter - variable */

#define CMD_PDELIMIT		0x28	/* Limitation of device power */
#define CMD_TEAM_FUNCTION	0x3C	/* Team function for PV inverters */

/* Global val */
extern smanet_session_t *smanet_session;

#define getshort(v) (short)( (((v)[1]) << 8) | (v)[0] )
#define putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p+0)) = ((int)(v) & 0xff); }
#define getlong(v) (uint32_t)( (((v)[3]) << 24) | (((v)[2]) << 16) | (((v)[1]) << 8) | (((v)[0]) & 0xff) )

/* internal funcs */
int smanet_read_frame(smanet_session_t *, uint8_t *, int, int);
int smanet_write_frame(smanet_session_t *, uint8_t *, int);

smanet_packet_t *smanet_alloc_packet(int);
void smanet_free_packet(smanet_packet_t *);
int smanet_recv_packet(smanet_session_t *, uint8_t, int, smanet_packet_t *, int);
int smanet_send_packet(smanet_session_t *s, uint16_t src, uint16_t dest, uint8_t ctrl, uint8_t cnt, uint8_t cmd, uint8_t *buffer, int buflen);

int smanet_command(smanet_session_t *s, int cmd, smanet_packet_t *p, uint8_t *buf, int buflen);
int smanet_get_net(smanet_session_t *s);
int smanet_get_net_start(smanet_session_t *s, long *sn, char *type, int len);
int smanet_cfg_net_adr(smanet_session_t *s, int);
int smanet_syn_online(smanet_session_t *s);

#define smanet_dlevel 6

#if 0
#define _gets8(v) (char)( ((char)(v)[0]) )
#define _gets16(v) (short)( ((short)(v)[1]) << 8 | ((short)(v)[0]) )
#define _gets32(v) (long)( ((long)(v)[3]) << 24 | ((long)(v)[2]) << 16 | ((long)(v)[1]) << 8 | ((long)(v)[0]) )
#define _gets64(v) (long long)( ((long long)(v)[7]) << 56 | ((long long)(v)[6]) << 48 | ((long long)(v)[5]) << 40 | ((long long)(v)[4]) << 32 | ((long long)(v)[3]) << 24 | ((long long)(v)[2]) << 16 | ((long long)(v)[1]) << 8 | ((long long)(v)[0]) )

#define _getu8(v) (unsigned char)( ((unsigned char)(v)[0]) )
#define _getu16(v) (unsigned short)( ((unsigned short)(v)[1]) << 8 | ((unsigned short)(v)[0]) )
#define _getu32(v) (unsigned long)( ((unsigned long)(v)[3]) << 24 | ((unsigned long)(v)[2]) << 16 | ((unsigned long)(v)[1]) << 8 | ((unsigned long)(v)[0]) )
#define _getu64(v) (unsigned long long)( ((unsigned long long)(v)[7]) << 56 | ((unsigned long long)(v)[6]) << 48 | ((unsigned long long)(v)[5]) << 40 | ((unsigned long long)(v)[4]) << 32 | ((unsigned long long)(v)[3]) << 24 | ((unsigned long long)(v)[2]) << 16 | ((unsigned long long)(v)[1]) << 8 | ((unsigned long long)(v)[0]) )

//#define _getf32(v) (float)( ((long)(v)[3]) << 24 | ((long)(v)[2]) << 16 | ((long)(v)[1]) << 8 | ((long)(v)[0]) )
//#define _getf32(v) (float)( ((int)(v)[3]) << 24 | ((int)(v)[2]) << 16 | ((int)(v)[1]) << 8 | ((int)(v)[0]) )
static inline float _getf32(unsigned char *v) {
	long val = ( ((long)v[3]) << 24 | ((long)v[2]) << 16 | ((long)v[1]) << 8 | ((long)v[0]) );
	return *(float *)&val;
}
static inline float _getf64(unsigned char *v) {
	long long val = ( ((long long)(v)[7]) << 56 | ((long long)(v)[6]) << 48 | ((long long)(v)[5]) << 40 | ((long long)(v)[4]) << 32 | ((long long)(v)[3]) << 24 | ((long long)(v)[2]) << 16 | ((long long)(v)[1]) << 8 | ((long long)(v)[0]) );
	return *(double *)&val;
}
#endif

#endif
