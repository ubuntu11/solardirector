
#ifndef __SMANET_INTERNAL_H
#define __SMANET_INTERNAL_H

#include <stdint.h>
#include "module.h"
#include "smanet.h"
#include "types.h"
#include "command.h"

struct smanet_session {
	list channels;
	solard_module_t *tp;
	void *tp_handle;
	long serial;
	char type[32];
	int addr;
	int timeouts;
	int commands;
};

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
int smanet_recv_packet(smanet_session_t *, int, smanet_packet_t *, int);
int smanet_send_packet(smanet_session_t *s, uint16_t src, uint16_t dest, uint8_t ctrl, uint8_t cnt, uint8_t cmd, uint8_t *buffer, int buflen);

int smanet_command(smanet_session_t *s, int cmd, smanet_packet_t *p, uint8_t *buf, int buflen);
int smanet_get_net_start(smanet_session_t *s, long *sn, char *type, int len);
int smanet_cfg_net_adr(smanet_session_t *s, int);
int smanet_syn_online(smanet_session_t *s);

#endif
