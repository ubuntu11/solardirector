
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"

int smanet_command(smanet_session_t *s, int cmd, smanet_packet_t *p, uint8_t *buf, int buflen) {
	int control,count,r;

	dprintf(smanet_dlevel,"src: %04x, dest: %04x, cmd: %d, packet: %p, buf: %p, buflen: %d\n", s->src, s->dest, cmd, p, buf, buflen);

	if (!p) return 1;
	if (cmd <= 0 || cmd > 60) return 1;

	r = 0;
	count = 0;
	s->timeouts = 0;
	control = s->dest ? 0 : 0x80;
	while(1) {
		s->commands++;
		if (smanet_send_packet(s,s->src,s->dest,control,count,cmd,buf,buflen)) return 1;
		/* Optimized for 19200 */
		usleep(550000);
		r = smanet_recv_packet(s,count,cmd,p,0);
		dprintf(smanet_dlevel,"r: %d\n", r);
		if (r < 0) break;
		if (r == 1) break;
		if (r == 2) {
			s->timeouts++;
			if (s->timeouts > 4) break;
			continue;
		}
		if (r == 3) continue;
		if (!p->response) continue;
		s->timeouts = 0;
		dprintf(smanet_dlevel,"p->command: %02x, cmd: %02x\n", p->command, cmd);
		if (p->command != cmd) continue;
		count = p->count;
		dprintf(smanet_dlevel,"count: %d\n",count);
		if (!count) break;
	}
	dprintf(smanet_dlevel,"returning: %d\n", r);
	return r;
}

int smanet_get_net_start(smanet_session_t *s, long *sn, char *type, int typesz) {
	smanet_packet_t *p;

	p = smanet_alloc_packet(12);
	if (!p) return 0;

	s->dest = 0;
	if (smanet_command(s,CMD_GET_NET_START,p,0,0)) return 1;
	*sn = getlong(p->data);
	dprintf(smanet_dlevel,"type: %p, buflen: %d\n", type, typesz);
	if (type && typesz) {
		type[0] = 0;
		strncat(type,(char *)&p->data[4],8);
		dprintf(smanet_dlevel,"type: %s\n", type);
	}
	smanet_free_packet(p);
	return 0;
}

int smanet_cfg_net_adr(smanet_session_t *s, int addr) {
	smanet_packet_t *p;
	uint8_t data[6];

	p = smanet_alloc_packet(4);
	if (!p) return 0;

	dprintf(smanet_dlevel,"setting addr to %d\n", addr);
	putlong(&data[0],s->serial);
	putlong(&data[4],addr);
	if (smanet_command(s,CMD_CFG_NETADR,p,data,sizeof(data))) return 1;
	s->dest = addr;
	smanet_free_packet(p);
	return 0;
}

int smanet_syn_online(smanet_session_t *s) {
	uint8_t data[4];

	time((time_t *)data);
	return smanet_send_packet(s,s->src,0,0x80,0,CMD_SYN_ONLINE,data,sizeof(data));
}
