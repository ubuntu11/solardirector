
#include "smanet_internal.h"

int smanet_command(smanet_session_t *s, int cmd, smanet_packet_t *p, uint8_t *buf, int buflen) {
	int control,count,r;

	dprintf(1,"cmd: %d, packet: %p, buf: %p, buflen: %d\n", cmd, p, buf, buflen);

	if (!p) return 1;
	if (cmd <= 0 || cmd > 60) {
//		smanet_seterr(s,"invalid command");
		return 1;
	}

#if 0
        sptr = data;
        eptr = data + DATASZ;
        count = 0;
        do {
                if (smanet_command(s,CMD_GET_CINFO,count,&p,0,0)) return 1;
                dprintf(1,"data_len: %d\n", p.data_len);
                if (sptr + p.data_len > eptr) return 1;
                memcpy(sptr,p.data,p.data_len);
                sptr += p.data_len;
                count = p.count;
        } while(count);
#endif

	count = 0;
	s->timeouts = 0;
	do {
		s->commands++;
		control = s->addr ? 0 : 0x80;
		if (smanet_send_packet(s,0,s->addr,control,count,cmd,buf,buflen)) return 1;
		usleep(550000);
		r = smanet_recv_packet(s,cmd,p,0);
		if (r < 0) return 1;
		if (r == 1) return 1;
		if (r == 2) {
			s->timeouts++;
			if (s->timeouts > 4) return 2;
			continue;
		}
		if (r == 3) continue;
		if (!p->response) continue;
                dprintf(1,"dataidx: %d\n", p->dataidx);
		count = p->count;
	} while(count);
	dprintf(1,"returning: %d\n", 0);
	return 0;
}

int smanet_get_net_start(smanet_session_t *s, long *sn, char *type, int typesz) {
	smanet_packet_t *p;

	p = smanet_alloc_packet(12);
	if (!p) return 0;

	s->addr = 0;
	if (smanet_command(s,CMD_GET_NET_START,p,0,0)) return 1;
	*sn = getlong(p->data);
	dprintf(1,"type: %p, buflen: %d\n", type, typesz);
	if (type && typesz) {
		type[0] = 0;
		strncat(type,(char *)&p->data[4],8);
		dprintf(1,"type: %s\n", type);
	}
	smanet_free_packet(p);
	return 0;
}

int smanet_cfg_net_adr(smanet_session_t *s, int addr) {
	smanet_packet_t *p;
	uint8_t data[6];

	p = smanet_alloc_packet(4);
	if (!p) return 0;

	putlong(&data[0],s->serial);
	putlong(&data[4],addr);
	if (smanet_command(s,CMD_CFG_NETADR,p,data,sizeof(data))) return 1;
	dprintf(1,"setting addr to %d\n", addr);
	s->addr = addr;
	smanet_free_packet(p);
	return 0;
}

int smanet_syn_online(smanet_session_t *s) {
	uint8_t data[4];

	time((time_t *)data);
	return smanet_send_packet(s,0,s->addr,0x80,0,CMD_SYN_ONLINE,data,sizeof(data));
}
