
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"

void jbd_get_protect(struct jbd_protect *p, unsigned short bits) {
#ifdef DEBUG
	{
		char bitstr[40];
		unsigned short mask = 1;
		int i;

		i = 0;
		while(mask) {
			bitstr[i++] = ((bits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bitstr[i] = 0;
		dprintf(5,"protect: %s\n",bitstr);
	}
#endif
#if 0
Bit0 monomer overvoltage protection
Bit1 monomer under voltage protection
Bit2 whole group overvoltage protection
Bit3 whole group under voltage protection
Bit4 charging over temperature protection
Bit5 charging low temperature protection
Bit6 discharge over temperature protection
Bit7 discharge low temperature protection
Bit8 charging over-current protection
Bit9 discharge over current protection
Bit10 short circuit protection
Bit11 front end detection IC error
Bit12 software lock MOS
Bit13 ~ bit15 reserved
bit0	......
Single overvoltage protection
bit1	......
Single undervoltage protection
bit2	......
Whole group overvoltage protection
bit3	......
Whole group undervoltage protection
bit4	......
Charge over temperature protection
bit5	......
Charge low temperature protection
bit6	......
Discharge over temperature protection
bit7	......
Discharge low temperature protection
bit8	......
Charge overcurrent protection
bit9	......
Discharge overcurrent protection
bit10	....
Short circuit protection
bit11	....IC..
Front detection IC error
bit12	....MOS
Software lock MOS
      bit13~bit15	..
Reserved

#endif
}

/* Read fetstate into s->fetstate */
int jbd_can_get_fetstate(struct jbd_session *s) {
	uint8_t data[8];

	/* 0x103 FET control status, production date, software version */
	if (jbd_can_get_crc(s,0x103,data,8)) return 1;
	s->fetstate = jbd_getshort(&data[0]);
	dprintf(2,"fetstate: %02x\n", s->fetstate);
	return 0;
}

/* For CAN bus only */
int jbd_can_read(struct jbd_session *s) {
	jbd_data_t *dp = &s->data;
	uint8_t data[8];
	int id,i;
	uint16_t protectbits;
	struct jbd_protect prot;

	/* 0x102 Equalization state low byte, equalized state high byte, protection status */
	if (jbd_can_get_crc(s,0x102,data,8)) return 1;
	dp->balancebits = jbd_getshort(&data[0]);
	dp->balancebits |= jbd_getshort(&data[2]) << 16;
	protectbits = jbd_getshort(&data[4]);
	/* Do we have any protection actions? */
	if (protectbits) {
		jbd_get_protect(&prot,protectbits);
	}

	/* 0x103 FET control status, production date, software version */
	if (jbd_can_get_crc(s,0x103,data,8)) return 1;
	s->fetstate = jbd_getshort(&data[0]);
	dprintf(2,"s->fetstate: %02x\n", s->fetstate);
	if (s->fetstate & JBD_MOS_CHARGE) solard_set_state(s,JBD_STATE_CHARGING);
	else solard_clear_state(s,JBD_STATE_CHARGING);
	if (s->fetstate & JBD_MOS_DISCHARGE) solard_set_state(s,JBD_STATE_DISCHARGING);
	else solard_clear_state(s,JBD_STATE_DISCHARGING);

	if (jbd_can_get_crc(s,0x104,data,8)) return 1;
	dp->ncells = data[0];
	dprintf(1,"strings: %d\n", dp->ncells);
	dp->ntemps = data[1];
	dprintf(1,"probes: %d\n", dp->ntemps);

	/* Get Temps */
	i = 0;
#define CTEMP(v) ( (v - 2731) / 10 )
	for(id = 0x105; id < 0x107; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		dp->temps[i++] = CTEMP((float)jbd_getshort(&data[0]));
		if (i >= dp->ntemps) break;
		dp->temps[i++] = CTEMP((float)jbd_getshort(&data[2]));
		if (i >= dp->ntemps) break;
		dp->temps[i++] = CTEMP((float)jbd_getshort(&data[4]));
		if (i >= dp->ntemps) break;
	}

	/* Cell volts */
	i = 0;
	for(id = 0x107; id < 0x111; id++) {
		if (jbd_can_get_crc(s,id,data,8)) return 1;
		dp->cellvolt[i++] = (float)jbd_getshort(&data[0]) / 1000;
		if (i >= dp->ncells) break;
		dp->cellvolt[i++] = (float)jbd_getshort(&data[2]) / 1000;
		if (i >= dp->ncells) break;
		dp->cellvolt[i++] = (float)jbd_getshort(&data[4]) / 1000;
		if (i >= dp->ncells) break;
	}

	return 0;
}

static int jbd_std_get_fetstate(jbd_session_t *s) {
	uint8_t data[128];
	int bytes;

	dprintf(3,"getting HWINFO...\n");
	if ((bytes = jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data))) < 0) {
		dprintf(1,"returning 1!\n");
		return 1;
	}
	s->fetstate = data[20];
	dprintf(2,"fetstate: %02x\n", s->fetstate);
	return 0;
}

int jbd_std_read(jbd_session_t *s) {
	jbd_data_t *dp = &s->data;
	uint8_t data[128];
	int i,bytes;;
	struct jbd_protect prot;

	dprintf(3,"getting HWINFO...\n");
	if ((bytes = jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data))) < 0) {
		dprintf(1,"returning 1!\n");
		return 1;
	}

	dp->voltage = (float)jbd_getshort(&data[0]) / 100.0;
	dp->current = (float)jbd_getshort(&data[2]) / 100.0;
	dp->capacity = (float)jbd_getshort(&data[6]) / 100.0;
        dprintf(2,"voltage: %.2f\n", dp->voltage);
        dprintf(2,"current: %.2f\n", dp->current);
        dprintf(2,"capacity: %.2f\n", dp->capacity);

	/* Balance */
	dp->balancebits = jbd_getshort(&data[12]);
	dp->balancebits |= jbd_getshort(&data[14]) << 16;
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((dp->balancebits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(5,"balancebits: %s\n",bits);
	}
#endif

	/* Protectbits */
	jbd_get_protect(&prot,jbd_getshort(&data[16]));

	s->fetstate = data[20];
	dprintf(2,"s->fetstate: %02x\n", s->fetstate);
	if (s->fetstate & JBD_MOS_CHARGE) solard_set_state(s,JBD_STATE_CHARGING);
	else solard_clear_state(s,JBD_STATE_CHARGING);
	if (s->fetstate & JBD_MOS_DISCHARGE) solard_set_state(s,JBD_STATE_DISCHARGING);
	else solard_clear_state(s,JBD_STATE_DISCHARGING);

	dp->ncells = data[21];
	dprintf(2,"cells: %d\n", dp->ncells);
	dp->ntemps = data[22];

	/* Temps */
#define CTEMP(v) ( (v - 2731) / 10 )
	for(i=0; i < dp->ntemps; i++) {
		dp->temps[i] = CTEMP((float)jbd_getshort(&data[23+(i*2)]));
	}

	/* Cell volts */
	if ((bytes = jbd_rw(s, JBD_CMD_READ, JBD_CMD_CELLINFO, data, sizeof(data))) < 0) return 1;
	for(i=0; i < dp->ncells; i++) dp->cellvolt[i] = (float)jbd_getshort(&data[i*2]) / 1000;

#ifdef DEBUG
	for(i=0; i < dp->ncells; i++) dprintf(2,"cell[%d]: %.1f\n", i, dp->cellvolt[i]);
#endif

	return 0;
}

int jbd_get_fetstate(jbd_session_t *s) {
	int r;

	dprintf(2,"transport: %s\n", s->tp->name);
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jbd_can_get_fetstate(s);
	else
		r = jbd_std_get_fetstate(s);
	return r;
}

int jbd_read(void *handle, void *buf, int buflen) {
	jbd_session_t *s = handle;

	if (!s->reader) {
		if (!s->tp) {
			log_error("jbd_read: tp is null!\n");
			return 1;
		}
		dprintf(2,"transport: %s\n", s->tp->name);
		if (strncmp(s->tp->name,"can",3)==0) 
			s->reader = jbd_can_read;
		else
			s->reader = jbd_std_read;
	}
	if (s->reader(s)) return 1;
	if (s->balancing) solard_set_state(s,JBD_STATE_BALANCING);
	else solard_clear_state(s,JBD_STATE_BALANCING);
	return 0;
}
