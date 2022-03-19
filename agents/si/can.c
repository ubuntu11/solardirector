
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_BITS 1

//#define TARGET_ENDIAN BIG_ENDIAN
#include "types.h"
#include "si.h"
#include <pthread.h>
#ifndef __WIN32
#include <sys/signal.h>
#endif
#include "transports.h"
#include <math.h>

#define dlevel 1

solard_driver_t *si_transports[] = { &can_driver, &rdev_driver, 0 };

#define BDATA(f,b) ((int8_t *) &s->frames[f].data[b]);
#define UBDATA(f,b) ((uint8_t *) &s->frames[f].data[b]);
#define SDATA(f,b) ((int16_t *) &s->frames[f].data[b]);
#define USDATA(f,b) ((uint16_t *) &s->frames[f].data[b]);
#define LDATA(f,b) ((int32_t *) &s->frames[f].data[b]);

static void _setraw(si_session_t *s) {
	struct si_raw_data *rdp = &s->raw_data;

	/* x300 Active power grid/gen */
	rdp->active_grid_l1 = SDATA(0,0);
	rdp->active_grid_l2 = SDATA(0,2);
	rdp->active_grid_l3 = SDATA(0,4);

	/* x301 Active power Sunny Island */
	rdp->active_si_l1 = SDATA(1,0);
	rdp->active_si_l2 = SDATA(1,2);
	rdp->active_si_l3 = SDATA(1,4);

	/* x302 Reactive power grid/gen */
	rdp->reactive_grid_l1 = SDATA(2,0);
	rdp->reactive_grid_l2 = SDATA(2,2);
	rdp->reactive_grid_l3 = SDATA(2,4);

	/* x303 Reactive power Sunny Island */
	rdp->reactive_si_l1 = SDATA(3,0);
	rdp->reactive_si_l2 = SDATA(3,2);
	rdp->reactive_si_l3 = SDATA(3,4);

	/* 0x304 AC1 Voltage L1 / AC1 Voltage L2 / AC1 Voltage L3 / AC1 Frequency */
	rdp->ac1_voltage_l1 = SDATA(4,0);
	rdp->ac1_voltage_l2 = SDATA(4,2);
	rdp->ac1_voltage_l3 = SDATA(4,4);
	rdp->ac1_frequency = SDATA(4,6);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	rdp->battery_voltage = SDATA(5,0);
	rdp->battery_current = SDATA(5,2);
	rdp->battery_temp = SDATA(5,4);
	rdp->battery_soc = SDATA(5,6);

	/* 0x306 SOH battery / Charging procedure / Operating state / active Error Message / Battery Charge Voltage Set-point */
	rdp->battery_soh = SDATA(6,0);
	rdp->charging_proc = BDATA(6,2);
	rdp->state = BDATA(6,3);
	rdp->errmsg = SDATA(6,4);
	rdp->battery_cvsp = SDATA(6,6);

	/* 0x307 Relay state / Relay function bit 1 / Relay function bit 2 / Synch-Bits */
	rdp->relay_state = USDATA(7,0);
	rdp->relay_bits1 = USDATA(7,2);
	rdp->relay_bits2 = USDATA(7,4);
	rdp->synch_bits = UBDATA(7,6);

	/* 0x308 TotLodPwr */
	rdp->TotLodPwr = SDATA(8,0);

	/* 0x309 AC2 Voltage L1 / AC2 Voltage L2 / AC2 Voltage L3 / AC2 Frequency */
	rdp->ac2_voltage_l1 = SDATA(9,0);
	rdp->ac2_voltage_l2 = SDATA(9,2);
	rdp->ac2_voltage_l3 = SDATA(9,4);
	rdp->ac2_frequency = SDATA(9,6);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	rdp->PVPwrAt = SDATA(10,0);
	rdp->GdCsmpPwrAt = SDATA(10,2);
	rdp->GdFeedPwr = SDATA(10,4);
}

#define GETFVAL(val) (float)_gets16(val)
#define GETFS100(val) (((float)_gets16(val))*100)
#define GETF100(val) (((float)_gets16(val))*100)
#define GETD10(val) (((float)_gets16(val))/10)
#define GETD100(val) (((float)_gets16(val))/100)

static int _getcur(si_session_t *s, char *what, si_current_source_t *spec, float *dest) {
	uint8_t data[8];
	float val;

	dprintf(dlevel,"spec: id: %03x, offset: %d, size: %d, type: %d, mult: %f\n",
		spec->can.id, spec->can.offset, spec->can.size, spec->type, spec->mult);
	if (spec->can.offset > 7) {
		log_warning("%s_current_source offset > 7, using 0\n",what);
		spec->can.offset = 0;
	}
	if (s->can_read(s,spec->can.id,data,8)) return 1;
	switch(spec->can.size) {
	case 1:
		val = data[spec->can.offset];
		break;
	case 2:
		val = _getshort(&data[spec->can.offset]);
		break;
	case 4:
		val = _getlong(&data[spec->can.offset]);
		break;
	default:
		val = 0;
		log_warning("%s_current_source size must be 1,2, or 4",what);
		break;
	}
	if (!fequal(spec->mult,0.0)) val *= spec->mult;
	dprintf(dlevel,"val: %.1f\n", val);
	*dest = val;
	return 0;
}

void si_can_get_relays(si_session_t *s) {
	si_raw_data_t *rdp = &s->raw_data;

#define RS(m,b) { s->data.m = ((_gets16(rdp->relay_state) & b) != 0); }
	RS(relay1,	0x0001);
	RS(relay2,	0x0002);
	RS(s1_relay1,	0x0004);
	RS(s1_relay2,	0x0008);
	RS(s2_relay1,	0x0010);
	RS(s2_relay2,	0x0020);
	RS(s3_relay1,	0x0040);
	RS(s3_relay2,	0x0080);
	RS(GnRn,	0x0100);
	RS(s1_GnRn,	0x0200);
	RS(s2_GnRn,	0x0400);
	RS(s3_GnRn,	0x0800);

	s->data.GnOn = (s->data.GnRn && s->data.AutoGn && s->data.ac1_frequency > 45.0) ? 1 : 0;

	/* 2-3 Relay function bit 1 */
#define RB1(m,b) { s->data.m = ((_gets16(rdp->relay_bits1) & b) != 0); }
	RB1(AutoGn,	0x0001);
	RB1(AutoLodExt,	0x0002);
	RB1(AutoLodSoc,	0x0004);
	RB1(Tm1,	0x0008);
	RB1(Tm2,	0x0010);
	RB1(ExtPwrDer,	0x0020);
	RB1(ExtVfOk,	0x0040);
	RB1(GdOn,	0x0080);
	RB1(Error,	0x0100);
	RB1(Run,	0x0200);
	RB1(BatFan,	0x0400);
	RB1(AcdCir,	0x0800);
	RB1(MccBatFan,	0x1000);
	RB1(MccAutoLod,	0x2000);
	RB1(Chp,	0x4000);
	RB1(ChpAdd,	0x8000);

	/* 4 & 5 Relay function bit 2 */
#define RB2(m,b) { s->data.m = ((_gets16(rdp->relay_bits2) & b) != 0); }
	RB2(SiComRemote,0x0001);
	RB2(OverLoad,	0x0002);
	RB2(ExtSrcConn,	0x0100);
	RB2(Silent,	0x0200);
	RB2(Current,	0x0400);
	RB2(FeedSelfC,	0x0800);
	RB2(Esave,	0x1000);
}

int si_can_get_data(si_session_t *s) {
	si_raw_data_t *rdp = &s->raw_data;

//	printf("\n\n***** GETTING DATA *****\n\n\n");

	s->data.active_grid_l1 = GETF100(rdp->active_grid_l1);
	s->data.active_grid_l1 = GETF100(rdp->active_grid_l1);
	s->data.active_grid_l2 = GETF100(rdp->active_grid_l2);
	s->data.active_grid_l3 = GETF100(rdp->active_grid_l3);

	s->data.active_si_l1 = GETF100(rdp->active_si_l1);
	s->data.active_si_l2 = GETF100(rdp->active_si_l2);
	s->data.active_si_l3 = GETF100(rdp->active_si_l3);

	s->data.reactive_grid_l1 = GETF100(rdp->reactive_grid_l1);
	s->data.reactive_grid_l2 = GETF100(rdp->reactive_grid_l2);
	s->data.reactive_grid_l3 = GETF100(rdp->reactive_grid_l3);

	s->data.reactive_si_l1 = GETF100(rdp->reactive_si_l1);
	s->data.reactive_si_l2 = GETF100(rdp->reactive_si_l2);
	s->data.reactive_si_l3 = GETF100(rdp->reactive_si_l3);

	s->data.ac1_voltage_l1 =  GETD10(rdp->ac1_voltage_l1);
	s->data.ac1_voltage_l2 =  GETD10(rdp->ac1_voltage_l2);
	s->data.ac1_voltage_l3 =  GETD10(rdp->ac1_voltage_l3);
	s->data.ac1_voltage = s->data.ac1_voltage_l1 + s->data.ac1_voltage_l2;
	s->data.ac1_frequency = GETD100(rdp->ac1_frequency);

	s->data.battery_voltage =  GETD10(rdp->battery_voltage);
	s->data.battery_current = GETD10(rdp->battery_current);
	s->data.battery_power = s->data.battery_voltage * s->data.battery_current;
	s->data.battery_temp = GETD10(rdp->battery_temp);
	s->data.battery_soc = GETD10(rdp->battery_soc);

	s->data.battery_soh = GETFVAL(rdp->battery_soh);
	s->data.charging_proc = _gets8(rdp->charging_proc);
	s->data.state = _gets8(rdp->state);
	s->data.errmsg = GETFVAL(rdp->errmsg);
	s->data.battery_cvsp = GETD10(rdp->battery_cvsp);

	si_can_get_relays(s);

	s->data.TotLodPwr = GETFS100(rdp->TotLodPwr);

	s->data.ac2_voltage_l1 = GETD10(rdp->ac2_voltage_l1);
	s->data.ac2_voltage_l2 = GETD10(rdp->ac2_voltage_l2);
	s->data.ac2_voltage_l3 = GETD10(rdp->ac2_voltage_l3);
	s->data.ac2_voltage = s->data.ac2_voltage_l1 + s->data.ac2_voltage_l2;
	s->data.ac2_frequency = GETD100(rdp->ac2_frequency);

	s->data.PVPwrAt =  GETD10(rdp->PVPwrAt);
	s->data.GdCsmpPwrAt = GETD10(rdp->GdCsmpPwrAt);
	s->data.GdFeedPwr = GETD10(rdp->GdFeedPwr);

	if (s->input.source == CURRENT_SOURCE_CAN) {
		if (s->input.type == CURRENT_TYPE_WATTS) {
			if (_getcur(s, "input", &s->input, &s->data.ac2_power)) return 1;
			s->data.ac2_current = s->data.ac2_power / s->data.ac2_voltage_l1;
		} else {
			if (_getcur(s, "input", &s->input, &s->data.ac2_current)) return 1;
			s->data.ac2_power = s->data.ac2_current * s->data.ac2_voltage_l1;
		}
//		dprintf(dlevel,"ac2_current: %.1f, ac2_power: %.1f\n", s->data.ac2_current, s->data.ac2_power);
	} else if (s->input.source == CURRENT_SOURCE_CALCULATED) {
		s->data.ac2_power = s->data.active_grid_l1 + s->data.active_grid_l2;
		s->data.ac2_power += (s->data.reactive_grid_l1*(-1)) + (s->data.reactive_grid_l2 *(-1));
		s->data.ac2_current = s->data.ac2_power / s->data.ac2_voltage;
//		dprintf(dlevel,"ac2_current: %.1f, ac2_power: %.1f\n", s->data.ac2_current, s->data.ac2_power);
	}

	if (s->output.source == CURRENT_SOURCE_CAN) {
		if (s->output.type == CURRENT_TYPE_WATTS) {
			if (_getcur(s, "output", &s->output, &s->data.ac1_power)) return 1;
			s->data.ac1_current = s->data.ac1_power / s->data.ac1_voltage_l1;
		} else {
			if (_getcur(s, "output", &s->output, &s->data.ac1_current)) return 1;
			s->data.ac1_power = s->data.ac1_current * s->data.ac1_voltage_l1;
		}
//		dprintf(dlevel,"ac1_current: %.1f, ac1_power: %.1f\n", s->data.ac1_current, s->data.ac1_power);
	} else if (s->output.source == CURRENT_SOURCE_CALCULATED) {
		s->data.ac1_power = s->data.active_si_l1 + s->data.active_si_l2;
		s->data.ac1_power += (s->data.reactive_si_l1*(-1)) + (s->data.reactive_si_l2 *(-1));
		s->data.ac1_power *= (-1);
		s->data.ac1_current = s->data.ac1_power / s->data.ac1_voltage;
//		dprintf(dlevel,"ac1_current: %.1f, ac1_power: %.1f\n", s->data.ac1_current, s->data.ac1_power);
	}
	return 0;
}

static void *si_can_recv_thread(void *handle) {
	si_session_t *s = handle;
	struct can_frame frame;
	int bytes,fidx;
	uint32_t can_id,mask;
#ifndef __WIN32
	sigset_t set;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);
#endif

	can_id = 0xffff;
//	printf("===> thread started!\n");
	while(solard_check_state(s,SI_STATE_RUNNING)) {
//		dprintf(dlevel,"open: %d\n", solard_check_state(s,SI_STATE_OPEN));
		if (!solard_check_state(s,SI_STATE_OPEN)) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		bytes = s->can->read(s->can_handle,&can_id,&frame,sizeof(frame));
//		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 1) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
//		printf("frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30f) continue;
//		bindump("frame",&frame,sizeof(frame));
		fidx = frame.can_id - 0x300;
		mask = 1 << frame.can_id;
		memcpy(&s->frames[fidx],&frame,sizeof(frame));
		s->bitmap |= mask;
	}
	dprintf(dlevel,"returning!\n");
	return 0;
}

static int si_can_get_local(si_session_t *s, uint32_t id, uint8_t *data, int datasz) {
	uint32_t mask;
	int fidx,retries,len;

	dprintf(dlevel,"id: %03x, data: %p, len: %d\n", id, data, datasz);
	if (id < 0x300 || id > 0x30f) return 1;

	fidx = id - 0x300;
	mask = 1 << id;
	dprintf(dlevel,"id: %03x, fidx: %x, mask: %08x, bitmap: %08x\n", id, fidx, mask, s->bitmap);
	retries=5;
	while(retries--) {
		if ((s->bitmap & mask) == 0) {
			dprintf(dlevel,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		len = (datasz > 8 ? 8 : datasz);
		memcpy(data,s->frames[fidx].data,len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
static int si_can_get_remote(si_session_t *s, uint32_t id, uint8_t *data, int datasz) {
	int retries,bytes,len;
	struct can_frame frame;

	dprintf(6,"id: %03x, data: %p, data_len: %d\n", id, data, datasz);
	retries=5;
	while(retries--) {
		bytes = s->can->read(s->can_handle,&id,&frame,sizeof(frame));
		dprintf(6,"bytes read: %d\n", bytes);
		if (bytes < 0) return 1;
		dprintf(6,"sizeof(frame): %d\n", sizeof(frame));
		if (bytes == sizeof(frame)) {
			len = (frame.can_dlc > datasz ? datasz : frame.can_dlc);
			memcpy(data,&frame.data,len);
//			if (debug >= 7) bindump("FROM DRIVER",data,len);
			break;
		}
		sleep(1);
	}
	dprintf(6,"returning: %d\n", (retries > 0 ? 0 : 1));
	return (retries > 0 ? 0 : 1);
}

int si_can_set_reader(si_session_t *s) {
	/* Start background recv thread */
	dprintf(dlevel,"driver name: %s\n", s->can->name);
	if (strcmp(s->can->name,"can") == 0) {
		pthread_attr_t attr;

		/* Set the filter to our range */
		s->can->config(s->can_handle,CAN_CONFIG_SET_RANGE,0x300,0x30F);

		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			sprintf(s->errmsg,"pthread_attr_init: %s",strerror(errno));
			goto si_can_set_reader_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			sprintf(s->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
			goto si_can_set_reader_error;
		}
		solard_set_state(s,SI_STATE_RUNNING);
		if (pthread_create(&s->th,&attr,&si_can_recv_thread,s)) {
			sprintf(s->errmsg,"pthread_create: %s",strerror(errno));
			goto si_can_set_reader_error;
		}
		pthread_attr_destroy(&attr);

		dprintf(dlevel,"setting func to local data\n");
		s->can_read = si_can_get_local;
	} else {
		dprintf(dlevel,"setting func to remote data\n");
		s->can_read = si_can_get_remote;
	}
	return 0;
si_can_set_reader_error:
	s->can_read = si_can_get_remote;
	return 1;
}

int si_can_init(si_session_t *s) {

	if (s->can) {
		/* Clear any previous filters */
		s->can->config(s->can_handle,CAN_CONFIG_CLEAR_FILTER);

		/* Stop any buffering */
		s->can->config(s->can_handle,CAN_CONFIG_STOP_BUFFER);

		if (strcmp(s->can->name,"can") == 0) {
			/* Stop the read thread and wait for it to exit */
			dprintf(dlevel,"SI_STATE_RUNNING: %d\n", solard_check_state(s,SI_STATE_RUNNING))
			if (solard_check_state(s,SI_STATE_RUNNING)) {
				solard_clear_state(s,SI_STATE_RUNNING);
				sleep(1);
			}
		}
		si_driver.close(s);
		s->can_init = false;
	}
	s->can_connected = false;

	/* Find the driver */
	s->can = find_driver(si_transports,s->can_transport);
	dprintf(dlevel,"s->can: %p\n", s->can);
	if (!s->can) {
		 if (strlen(s->can_transport)) {
			sprintf(s->errmsg,"unable to find CAN driver for transport: %s",s->can_transport);
			return 1;
		} else {
			sprintf(s->errmsg,"can_transport is empty!\n");
			return 1;
		}
	}

	/* Create new instance */
	dprintf(dlevel,"using driver: %s\n", s->can->name);
	s->can_handle = s->can->new(s->can_target, s->can_topts);
	dprintf(dlevel,"can_handle: %p\n", s->can_handle);
	if (!s->can_handle) {
		sprintf(s->errmsg,"unable to create new instance of %s driver: %s",
			s->can->name,strerror(errno));
		return 1;
	}
	if (si_can_set_reader(s)) return 1;

	/* Set the pointers into the raw data */
	if (!s->raw_data.active_grid_l1) _setraw(s);

	/* Open it */
	if (si_driver.open(s)) {
		sprintf(s->errmsg,"error opening can driver");
		return 1;
	}

	/* If local, "prime" the frames by reading a few IDs */
	if (strcmp(s->can->name,"can") == 0) {
		uint8_t data[8];

		if (s->can_read(s,0x304,data,8)) return 1;
		if (s->can_read(s,0x305,data,8)) return 1;
		if (s->can_read(s,0x307,data,8)) return 1;
		if (s->can_read(s,0x308,data,8)) return 1;
		if (s->can_read(s,0x309,data,8)) return 1;
	}

	/* OK, we're connected */
	s->can_connected = true;

	if (!strlen(s->input.text)) s->input.source = CURRENT_SOURCE_NONE;
	if (!strlen(s->input.text)) s->output.source = CURRENT_SOURCE_NONE;
	if (s->ap) s->ap->read_interval = s->ap->write_interval = s->interval = 10;
	s->can_init = true;
	return 0;
}

int si_can_read_relays(si_session_t *s) {
	if (s->can_read(s,0x307,s->frames[7].data,8)) return 1;
	si_can_get_relays(s);
	return 0;
}

int si_can_read_data(si_session_t *s, int all) {

	dprintf(2,"all: %d\n",all);

	/* If local can driver running, just refresh data */
	dprintf(2,"s->can->name: %s\n", s->can->name);
	if (strcmp(s->can->name,"rdev") != 0) return si_can_get_data(s);

	if (all || s->input.source == CURRENT_SOURCE_CALCULATED) {
		if (s->can_read(s,0x300,s->frames[0].data,8)) return 1;
		if (s->can_read(s,0x302,s->frames[2].data,8)) return 1;
	}

	if (all || s->output.source == CURRENT_SOURCE_CALCULATED) {
		if (s->can_read(s,0x301,s->frames[1].data,8)) return 1;
		if (s->can_read(s,0x303,s->frames[3].data,8)) return 1;
	}

	if (s->can_read(s,0x304,s->frames[4].data,8)) return 1;
	if (s->can_read(s,0x305,s->frames[5].data,8)) return 1;
	if (s->can_read(s,0x306,s->frames[6].data,8)) return 1;
	if (s->can_read(s,0x307,s->frames[7].data,8)) return 1;
	if (s->can_read(s,0x308,s->frames[8].data,8)) return 1;
	if (s->can_read(s,0x309,s->frames[9].data,8)) return 1;
	if (s->can_read(s,0x30a,s->frames[10].data,8)) return 1;

	si_can_get_data(s);
	return 0;
}

int si_can_write(si_session_t *s, uint32_t id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len,bytes;

	dprintf(dlevel,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

//	bindump("write data",data,data_len);
	dprintf(dlevel,"readonly: %d\n", s->readonly);
	if (s->readonly) return data_len;

	/* XXX disabled */
	printf("*** NOT WRITING ***\n");
	return data_len;

	len = data_len > 8 ? 8 : data_len;

	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,data,len);
//	bindump("write data",data,data_len);
	bytes = s->can->write(s->can_handle,&id,&frame,sizeof(frame));
	dprintf(dlevel,"bytes: %d\n", bytes);
	return bytes;
}

int si_can_write_data(si_session_t *s) {
	uint8_t data[8];
	float soc;

	if (!s->can_connected) return 0;

	/* Startup SOC/Charge amps */
	dprintf(1,"startup: %d\n", s->startup);
	if (s->startup == 1) {
		soc = 99.9;
		s->charge_amps = 1;
		s->startup = 2;
	} else if (s->startup == 2) {
		soc = s->soc;
		s->charge_amps = 0;
		s->startup = 0;
	} else {
		soc = s->soc;
	}

	/* Take charge amps to 0.0? need to go to 1 first then 0.0 next pass */
	dprintf(1,"tozero: %d, charge_amps: %.1f\n", s->tozero, s->charge_amps);
	if (s->tozero == 1) {
		if (float_equals(s->charge_amps,1.0)) {
			dprintf(dlevel,"setting charge amps to 0...\n");
			s->charge_amps = 0.0;
			s->tozero = 2;
		} else {
			s->tozero = 0;
		}
	} else {
		if (float_equals(s->charge_amps,0.0)) {
			if (s->tozero != 2) {
				s->charge_amps = 1;
				s->tozero = 1;
			}
		} else {
			s->tozero = 0;
		}
	}
	if (float_equals(s->discharge_amps,0.0)) {
		log_warning("discharge_amps is 0.0, setting to 1200.0\n");
		s->discharge_amps = 1200;
	}

	/* 0x350 Active Current Set-point / Reactive current Set-Point */

	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(dlevel,"0x351: charge_voltage: %.1f, charge_amps: %.1f, discharge_amps: %.1f, discharge_voltage: %.1f\n",
		s->charge_voltage, s->charge_amps, s->discharge_amps, s->min_voltage);
	_putshort(&data[0],(s->charge_voltage * 10.0));
	_putshort(&data[2],(s->charge_amps * 10.0));
	_putshort(&data[4],(s->discharge_amps * 10.0));
	_putshort(&data[6],(s->min_voltage * 10.0));
	if (si_can_write(s,0x351,data,8) < 0) return 1;

	/* 0x352 Nominal frequency (F0) */
	/* 0x353 Nominal voltage L1 (U0) */
	/* 0x354 Function Bits SiCom V1 */

	/* 0x355 SOC value / SOH value / HiResSOC */
	dprintf(dlevel,"0x355: SoC: %.1f, SoH: %.1f\n", soc, s->soh);
	_putshort(&data[0],soc);
	_putshort(&data[2],s->soh);
	_putlong(&data[4],(soc * 100.0));
	if (si_can_write(s,0x355,data,8) < 0) return 1;

#if 0
	/* 0x356 Battery Voltage / Battery Current / Battery Temperature */
	dprintf(dlevel,"0x356: battery_voltage: %3.2f, battery_amps: %3.2f, battery_temp: %3.2f\n",
		s->data.battery_voltage, s->data.battery_amps, s->data.battery_temp);
	_putshort(&data[0],s->data.battery_voltage * 100.0);
	_putshort(&data[2],s->data.battery_current * 10.0);
	if (s->have_battery_temp) {
		_putshort(&data[4],s->data.battery_temp * 10.0);
	} else {
		_putshort(&data[4],0);
	}
	if (si_can_write(s,0x356,data,8) < 0) return 1;
#endif

	/* 0x357 - 0x359 ?? */

	/* 0x35A Alarms / Warnings */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35A,data,8) < 0) return 1;

	/* 0x35B Events */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35B,data,8) < 0) return 1;

	/* 0x35C Function Bits / Function Bits Relays */

#if 0
	/* 0x35E Manufacturer-Name-ASCII (8 chars) */
	memset(data,0x20,sizeof(data));
	data[0] = 0x53; data[1] = 0x50; data[2] = 0x53;
	if (si_can_write(s,0x35E,data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	_putshort(&data[2],10000);
	_putshort(&data[4],2500);
	_putshort(&data[6],1);
	if (si_can_write(s,0x35F,data,8) < 0) return 1;
#endif

	/* 0x360 Limitation of external current (Gen/Grid) */

	return 0;
}
