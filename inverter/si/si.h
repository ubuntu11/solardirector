
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_SI_H
#define __SOLARD_SI_H

#include "agent.h"
#include "smanet.h"
#include "inverter.h"
#include <pthread.h>

enum SI_PARAM_SOURCE {
	SI_PARAM_SOURCE_UNK,
	SI_PARAM_SOURCE_INV,
	SI_PARAM_SOURCE_SI
};

struct si_param {
	char name[32];
	enum SI_PARAM_SOURCE source;
	enum DATA_TYPE type;
	void *ptr;
	int len;
	union {
		unsigned char bval;
		short wval;
		long lval;
		float fval;
		double dval;
//		char sval[128];
		char *sval;
	};
};
typedef struct si_param si_param_t;

struct si_session {
	solard_agent_t *ap;
	int readonly;
	char smanet_transport[SOLARD_TRANSPORT_LEN];
	char smanet_target[SOLARD_TARGET_LEN];
	char smanet_topts[SOLARD_TOPTS_LEN];
	smanet_session_t *smanet;
	char channels_path[256];
	list desc;
	pthread_t th;
	int startup;
	/* 0x300 to 0x30F */
	struct can_frame frames[16];
	uint32_t bitmap;
	char can_transport[SOLARD_TRANSPORT_LEN];
	char can_target[SOLARD_TARGET_LEN];
	char can_topts[SOLARD_TOPTS_LEN];
	solard_module_t *can;
	void *can_handle;
	int (*can_read)(struct si_session *, int id, uint8_t *data, int len);
	uint16_t state;
	json_proctab_t idata;
	si_param_t pdata;
	struct {
		unsigned relay1: 1;
		unsigned relay2: 1;
		unsigned s1_relay1: 1;
		unsigned s1_relay2: 1;
		unsigned s2_relay1: 1;
		unsigned s2_relay2: 1;
		unsigned s3_relay1: 1;
		unsigned s3_relay2: 1;
		unsigned GnRn: 1;
		unsigned s1_GnRn: 1;
		unsigned s2_GnRn: 1;
		unsigned s3_GnRn: 1;
		unsigned AutoGn: 1;
		unsigned AutoLodExt: 1;
		unsigned AutoLodSoc: 1;
		unsigned Tm1: 1;
		unsigned Tm2: 1;
		unsigned ExtPwrDer: 1;
		unsigned ExtVfOk: 1;
		unsigned GdOn: 1;
		unsigned Error: 1;
		unsigned Run: 1;
		unsigned BatFan: 1;
		unsigned AcdCir: 1;
		unsigned MccBatFan: 1;
		unsigned MccAutoLod: 1;
		unsigned Chp: 1;
		unsigned ChpAdd: 1;
		unsigned SiComRemote: 1;
		unsigned OverLoad: 1;
		unsigned ExtSrcConn: 1;
		unsigned Silent: 1;
		unsigned Current: 1;
		unsigned FeedSelfC: 1;
	} bits;
	int have_battery_temp;
	float charge_voltage;
	float charge_amps;
	float charge_min_amps;
	float save_charge_amps;
	int charge_mode;
	float start_temp;
	float user_soc;
	float tvolt;
	int sim;
	float sim_step;
	int interval;
	int charge_creep;
	int run_state;
	float last_battery_voltage, last_soc;
	float last_charge_voltage, last_battery_amps;
	time_t cv_start_time;
};
typedef struct si_session si_session_t;

#define SI_STATE_STARTUP	0x01
#define SI_STATE_RUNNING	0x02
#define SI_STATE_CHARGING	0x04
#define SI_STATE_OPEN		0x10

void *si_recv_thread(void *handle);
int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_read(si_session_t *,void *,int);
int si_can_write(si_session_t *s, int id, uint8_t *data, int data_len);
int si_write(si_session_t *,void *,int);
int si_control(void *handle,char *op, char *id, json_value_t *actions);
int si_config(void *,char *,char *,list);
json_value_t *si_info(void *);
int si_config_add_info(si_session_t *s, json_value_t *j);

/* Charging */
void charge_init(si_session_t *s);
void charge_max_start(si_session_t *s);
void charge_max_stop(si_session_t *s);
void charge_stop(si_session_t *s, int rep);
void charge_start(si_session_t *s, int rep);
void charge_start_cv(si_session_t *s, int rep);
void charge_check(si_session_t *s);

int si_startstop(si_session_t *s, int);

#define si_getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = ((int)(v) & 0x0F); }

#define SI_VOLTAGE_MIN	36.0
#define SI_VOLTAGE_MAX	64.0

//#define si_isvrange(v) ((v >= SI_VOLTAGE_MIN) && (v  <= SI_VOLTAGE_MAX))

#endif /* __SOLARD_SI_H */
