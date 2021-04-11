
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
		dprintf(8,"open: %d\n", solard_check_state(s,SI_STATE_OPEN));
		if (!solard_check_state(s,SI_STATE_OPEN)) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		bytes = s->can->read(s->can_handle,&frame,0xffff);
		dprintf(8,"bytes: %d\n", bytes);
		if (bytes < 1) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		dprintf(8,"frame.can_id: %03x\n",frame.can_id);
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
	uint8_t bits;

	dprintf(1,"charge_voltage: %.1f, discharge_voltage: %.1f\n", inv->charge_voltage, inv->discharge_voltage);
	if (!si_isvrange(inv->discharge_voltage) || !si_isvrange(inv->charge_voltage)) {
		log_write(LOG_ERROR,"charge_voltage and/or discharge_voltage not set\n");
		return 1;
	}

	/* 0x300 Active power grid/gen L1/L2/L3 */
	if (s->get_data(s,0x300,data,sizeof(data))) return 1;
	inv->grid_watts.l1 = (si_getshort(&data[0]) * 100.0);
	inv->grid_watts.l2 = (si_getshort(&data[2]) * 100.0);
	inv->grid_watts.l3 = (si_getshort(&data[4]) * 100.0);
	inv->grid_watts.total = inv->grid_watts.l1 + inv->grid_watts.l2 + inv->grid_watts.l3;
	dprintf(1,"grid_watts: %3.2f\n",inv->grid_watts.total);

	/* 0x301 Active power Sunny Island L1/L2/L3 */
	/* 0x302 Reactive power grid/gen L1/L2/L3 */
	/* 0x303 Reactive power Sunny Island L1/L2/L3 */

	/* 0x304 Voltage L1 Voltage L2 Voltage L3 Frequency */
	/* AC1 = My output */
	if (s->get_data(s,0x304,data,sizeof(data))) return 1;
	inv->load_voltage.l1 = (si_getshort(&data[0]) / 10.0);
	inv->load_voltage.l2 = (si_getshort(&data[2]) / 10.0);
	inv->load_voltage.l3 = (si_getshort(&data[4]) / 10.0);
	inv->load_voltage.total = inv->load_voltage.l1 + inv->load_voltage.l2 + inv->load_voltage.l3;
	inv->load_frequency = (si_getshort(&data[6]) / 100.0);
	dprintf(1,"load_voltage: %3.2f, frequency: %3.2f\n",inv->load_voltage.total, inv->load_frequency);

	/* 0x305 Battery voltage / Battery current / Battery temperature / SOC battery */
	if (s->get_data(s,0x305,data,sizeof(data))) return 1;
	inv->battery_voltage = si_getshort(&data[0]) / 10.0;
	inv->battery_current = si_getshort(&data[2]) / 10.0;
	inv->battery_temp = si_getshort(&data[4]) / 10.0;
	dprintf(1,"battery_voltage: %2.2f, battery_current: %2.2f, battery_temp: %2.1f\n",
		inv->battery_voltage, inv->battery_current, inv->battery_temp);
	inv->battery_power = inv->battery_current * inv->battery_voltage;
	dprintf(1,"battery_power: %3.2f\n",inv->battery_power);

	/* 0x306 SOH battery Charging procedure / Operating state / active Error Message / Battery Charge Volt-age Set-point */

	/* 0x307 Relay state / Relay function bit 1 / Relay function bit 2 / Synch-Bits */
	s->get_data(s,0x307,data,8);
#define SET(m,b) { s->bits.m = ((bits & b) != 0); dprintf(1,"bits.%s: %d\n",#m,s->bits.m); }
	bits = data[0];
	dump_bits("data[0]",bits);
	SET(relay1,   0x0001);
	SET(relay2,   0x0002);
	SET(s1_relay1,0x0004);
	SET(s1_relay2,0x0008);
	SET(s2_relay1,0x0010);
	SET(s2_relay2,0x0020);
	SET(s3_relay1,0x0040);
	SET(s3_relay2,0x0080);
	bits = data[1];
	dump_bits("data[1]",bits);
	SET(GnRn,     0x0001);
	SET(s1_GnRn,  0x0002);
	SET(s2_GnRn,  0x0004);
	SET(s3_GnRn,  0x0008);
	bits = data[2];
	dump_bits("data[2]",bits);
	SET(AutoGn,    0x0001);
	SET(AutoLodExt,0x0002);
	SET(AutoLodSoc,0x0004);
	SET(Tm1,       0x0008);
	SET(Tm2,       0x0010);
	SET(ExtPwrDer, 0x0020);
	SET(ExtVfOk,   0x0040);
	SET(GdOn,      0x0080);
	bits = data[3];
	dump_bits("data[3]",bits);
	SET(Error,     0x0001);
	SET(Run,       0x0002);
	SET(BatFan,    0x0004);
	SET(AcdCir,    0x0008);
	SET(MccBatFan, 0x0010);
	SET(MccAutoLod,0x0020);
	SET(Chp,       0x0040);
	SET(ChpAdd,    0x0080);
	bits = data[4];
	dump_bits("data[4]",bits);
	SET(SiComRemote,0x0001);
	SET(OverLoad,  0x0002);
	bits = data[5];
	dump_bits("data[5]",bits);
	SET(ExtSrcConn,0x0001);
	SET(Silent,    0x0002);
	SET(Current,   0x0004);
	SET(FeedSelfC, 0x0008);

	/* 0x308 TotLodPwr L1/L2/L3 */
	if (s->get_data(s,0x308,data,sizeof(data))) return 1;
	inv->load_watts.l1 = (si_getshort(&data[0]) * 100.0);
	inv->load_watts.l2 = (si_getshort(&data[2]) * 100.0);
	inv->load_watts.l3 = (si_getshort(&data[4]) * 100.0);
	inv->load_watts.total = inv->load_watts.l1 + inv->load_watts.l2 + inv->load_watts.l3;
	dprintf(1,"load_watts: %3.2f\n",inv->load_watts.total);

	/* 0x309 AC2 Voltage L1 AC2 Voltage L2 AC2 Voltage L3 AC2 Frequency */
	/* AC2 = Grid/Gen */
	if (s->get_data(s,0x309,data,sizeof(data))) return 1;
	inv->grid_voltage.l1 = (si_getshort(&data[0]) / 10.0);
	inv->grid_voltage.l2 = (si_getshort(&data[2]) / 10.0);
	inv->grid_voltage.l3 = (si_getshort(&data[4]) / 10.0);
	inv->grid_voltage.total = inv->grid_voltage.l1 + inv->grid_voltage.l2 + inv->grid_voltage.l3;
	inv->grid_frequency = (si_getshort(&data[6]) / 100.0);
	dprintf(1,"grid_voltage: %3.2f, frequency: %3.2f\n",inv->grid_voltage.total, inv->grid_frequency);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
//	s->get_data(s,0x30a,data,sizeof(data));

	/* Calc SOC if possible */

	inv->soc = s->soc < 0.0 ? ( ( inv->battery_voltage - inv->discharge_voltage) / (inv->charge_voltage - inv->discharge_voltage) ) * 100.0 : s->soc;
	lprintf(0,"SoC: %.1f\n", inv->soc);
	inv->soh = 100.0;
	return 0;
}
