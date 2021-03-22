
#include "si.h"
#include <pthread.h>
#include <linux/can.h>
#include <sys/signal.h>

void *si_recv_thread(void *handle) {
	si_session_t *s = handle;
	struct can_frame frame;
	sigset_t set;
	int bytes;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	dprintf(3,"thread started!\n");
	while(solard_check_state(s,SI_STATE_RUNNING)) {
		dprintf(7,"open: %d\n", solard_check_state(s,SI_STATE_OPEN));
		if (!solard_check_state(s,SI_STATE_OPEN)) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		bytes = s->can->read(s->can_handle,&frame,0xffff);
		dprintf(7,"bytes: %d\n", bytes);
		if (bytes < 1) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		dprintf(7,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30F) continue;
//		bindump("frame",&frame,sizeof(frame));
		memcpy(&s->frames[frame.can_id - 0x300],&frame,sizeof(frame));
		s->bitmap |= 1 << (frame.can_id - 0x300);
	}
	dprintf(1,"returning!\n");
	return 0;
}

int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int datasz) {
	char what[16];
	uint16_t mask;
	int idx,retries,len;

	dprintf(5,"id: %03x, data: %p, len: %d\n", id, data, datasz);

	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x, bitmap: %04x\n", mask, s->bitmap);
	retries=5;
	while(retries--) {
		if ((s->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
//		if (debug >= 5) bindump(what,&s->frames[idx],sizeof(struct can_frame));
		len = (datasz > 8 ? 8 : datasz);
		memcpy(data,s->frames[idx].data,len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int datasz) {
	int retries,bytes,len;
	struct can_frame frame;

	dprintf(5,"id: %03x, data: %p, data_len: %d\n", id, data, datasz);

	retries=5;
	while(retries--) {
		bytes = s->can->read(s->can_handle,&frame,id);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return 1;
		if (bytes == sizeof(frame)) {
			len = (frame.can_dlc > datasz ? datasz : frame.can_dlc);
			memcpy(data,&frame.data,len);
			if (debug >= 5) bindump("FROM SERVER",data,len);
			break;
		}
		sleep(1);
	}
	dprintf(5,"returning: %d\n", (retries > 0 ? 0 : 1));
	return (retries > 0 ? 0 : 1);
}

void dump_bits(char *label, uint8_t bits) {
	register uint8_t i,mask;
	char bitstr[9];

	i = 0;
	mask = 1;
	while(mask) {
		bitstr[i++] = (bits & mask ? '1' : '0');
		mask <<= 1;
	}
	bitstr[i] = 0;
	dprintf(1,"%s(%02x): %s\n",label,bits,bitstr);
}

int si_read(si_session_t *s, void *buf, int buflen) {
	solard_inverter_t *inv = buf;
	uint8_t data[8];

	/* x300 Active power grid/gen L1/L2/L3 */
	if (s->get_data(s,0x300,data,sizeof(data))) return 1;
	inv->grid_power = (si_getshort(&data[0]) * 100.0) + (si_getshort(&data[2]) * 100.0) + (si_getshort(&data[4]) * 100.0);
	dprintf(1,"grid_power: %3.2f\n",inv->grid_power);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	if (s->get_data(s,0x305,data,sizeof(data))) return 1;
	inv->battery_voltage = si_getshort(&data[0]) / 10.0;
	inv->battery_current = si_getshort(&data[2]) / 10.0;
	dprintf(1,"battery_voltage: %2.2f, battery_current: %2.2f\n", inv->battery_voltage, inv->battery_current);
	inv->battery_power = inv->battery_current * inv->battery_voltage;
	dprintf(1,"battery_power: %3.2f\n",inv->battery_power);

	/* 0x308 TotLodPwr L1/L2/L3 */
	s->get_data(s,0x308,data,sizeof(data));
	inv->load_power = (si_getshort(&data[0]) * 100.0) + (si_getshort(&data[2]) * 100.0) + (si_getshort(&data[4]) * 100.0);
	dprintf(1,"load_power: %3.2f\n",inv->load_power);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	s->get_data(s,0x30a,data,sizeof(data));
	inv->site_power = si_getshort(&data[0]) * 100.0;
	dprintf(1,"site_power: %3.2f\n",inv->site_power);
	return 0;
}
