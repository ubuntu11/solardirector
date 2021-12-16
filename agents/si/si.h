
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
#include <pthread.h>

struct si_power {
	float l1;
	float l2;
	float l3;
};
typedef struct si_power si_power_t;

struct si_info {
	struct {
		si_power_t grid;
		si_power_t si;
	} active;
	struct {
		si_power_t grid;
		si_power_t si;
	} reactive;
	si_power_t volt;
	float voltage;
	float frequency;
	float ac1_current;
	float battery_voltage;
	float battery_current;
	float battery_temp;
	float battery_soc;
	float battery_soh;
	float battery_cvsp;
	int relay1;
	int relay2;
	int s1_relay1;
	int s1_relay2;
	int s2_relay1;
	int s2_relay2;
	int s3_relay1;
	int s3_relay2;
	int GnRn;
	int s1_GnRn;
	int s2_GnRn;
	int s3_GnRn;
	int AutoGn;
	int AutoLodExt;
	int AutoLodSoc;
	int Tm1;
	int Tm2;
	int ExtPwrDer;
	int ExtVfOk;
	int GdOn;
	int GnOn;
	int Error;
	int Run;
	int BatFan;
	int AcdCir;
	int MccBatFan;
	int MccAutoLod;
	int Chp;
	int ChpAdd;
	int SiComRemote;
	int OverLoad;
	int ExtSrcConn;
	int Silent;
	int Current;
	int FeedSelfC;
	int Esave;
	float TotLodPwr;
	uint8_t charging_proc;
	uint8_t state;
	uint16_t errmsg;
	si_power_t ac2_volt;
	float ac2_voltage;
	float ac2_frequency;
	float ac2_current;
	float PVPwrAt;
	float GdCsmpPwrAt;
	float GdFeedPwr;
};
typedef struct si_info si_info_t;

enum GRID_CONNECT_REASON {
	GRID_CONNECT_REASON_SOC = 501,		/* Grid request due to SOC (insufficient value) */
	GRID_CONNECT_REASON_PWR = 503,		/* Grid request due to exceeding the power limit */
	GRID_CONNECT_REASON_MAN = 505,		/* Manual grid request */
	GRID_CONNECT_REASON_FEED = 507,		/* Feed-in started */
};

enum GEN_CONNECT_REASON {
	GEN_CONNECT_REASON_AUTO = 401,		/* Automatic generator start due to set criteria */
	GEN_CONNECT_REASON_MAN = 403,		/* Manual generator start */
	GEN_CONNECT_REASON_CUR = 407,		/* Current-regulated generator operation initiated? */
};

/* history of battery amps */
#define SI_MAX_BA 6

struct si_session {
	solard_agent_t *ap;
	/* CAN */
	char can_transport[SOLARD_TRANSPORT_LEN];
	char can_target[SOLARD_TARGET_LEN];
	char can_topts[SOLARD_TOPTS_LEN];
	int can_fallback;
	solard_driver_t *can;
	void *can_handle;
	pthread_t th;
	/* 0x300 to 0x30F */
	struct can_frame frames[16];
	uint32_t bitmap;
	int (*can_read)(struct si_session *, int id, uint8_t *data, int len);
	si_info_t info;			/* CAN info will be read into this struct */
	/* SMANET */
	char smanet_transport[SOLARD_TRANSPORT_LEN];
	char smanet_target[SOLARD_TARGET_LEN];
	char smanet_topts[SOLARD_TOPTS_LEN];
	char smanet_channels_path[1024];
	smanet_session_t *smanet;
	char ExtSrc[16]; 		/* PvOnly, Gen, Grid, GenGrid */
	list desc;
	uint16_t state;
	int interval;
	int readonly;
	/* Charging */
	int startup_charge_mode;
	float max_voltage;		/* Dont go above this voltage */
	float min_voltage;		/* Dont go below this voltage */
	float charge_start_voltage;	/* Voltage to start charging (empty voltage) */
	float charge_end_voltage;	/* Voltage to end charging (full voltage) */
	int charge_at_max;		/* Charge at max_voltage until battery_voltage >= charge_end_voltage */
	int charge_creep;		/* Increase charge voltage until battery amps = charge amps */
	float charge_voltage;		/* RO|NOSAVE, charge_voltage */
	float charge_amps;		/* Charge amps */
	float last_ca;			/* Last charge amps */
	float charge_amps_temp_modifier; /* RO|NOSAVE, charge_amps temperature modifier */
	float charge_amps_soc_modifier;	/* RO|NOSAVE, charge_amps SoC modifier */
	float charge_min_amps;		/* Set charge_amps to this value when not charging */
	float std_charge_amps;
	int charge_mode;
	int charge_method;
	time_t cv_start_time;
	int cv_method;
	int cv_time;
	float cv_cutoff;		/* When average of last X amps reach below this, stop charging */
	float ba[SI_MAX_BA];		/* Table of last X amps during CV */
	int baidx;
	int bafull;
	float start_temp;

	/* Grid */
	int have_grid;
	/* EC start */
	int grid_started;		/* Grid started via GdManStr */
//	time_t grid_op_time;
//	int grid_stop_timeout;		/* How long to wait until Grid stopped */
	/* EC end */
	int grid_connected;		/* info.GdOn */
	enum GRID_CONNECT_REASON grid_connect_reason;	/* E501-E508 */
	/* Charge from grid parms */
	int grid_start_timeout;		/* How long to wait until Grid started */
	char grid_save[32];		/* Saved contents of GdManStr */
	int charge_from_grid;		/* Always charge when grid is connected? */
	float grid_charge_amps;		/* Charge amps to use when grid is connected */
	float grid_charge_start_voltage; /* Voltage to start charging when grid connected */
	float grid_charge_start_soc;	/* SoC to start charging when grid connected */
	float grid_charge_stop_voltage; /* Voltage to stop charging when grid connected */
	float grid_charge_stop_soc;	/* SoC to stop charging when grid connected */
	/* End charge from grid parms */
	int grid_soc_enabled;		/* Grid connected for SoC? (GdSocEna) */
	float grid_soc_stop;		/* When the grid will shutoff (GdSocTm1Stp) */

	/* Gen */
	int have_gen;
	/* EC start */
	int gen_started;		/* Generator started via GnManStr */
	time_t gen_op_time;		/* Time of last gen op */
	char gen_save[32];		/* Saved value of GnManStr */
	/* EC end */
	int gen_connected;		/* info.GnOn */
	enum GEN_CONNECT_REASON gen_connect_reason;	/* E501-E508 */
	float gen_charge_amps;		/* Charge amps to use when gen connected */
	float gen_soc_stop;		/* When the gen will auto shutoff (GnSocTm1Stp) */
//	int gen_start_timeout;
//	int gen_stop_timeout;

	float discharge_amps;
	float soc;
	float user_soc;
	float soh;
	/* Uhh */
	json_proctab_t idata;
	void *pdata;
	/* SIM */
	int sim;
	float tvolt;
	float sim_amps;
	float sim_step;
	/* Display/Reporting */
	float last_battery_voltage;
	float last_soc;
	float last_charge_voltage;
	float last_battery_amps;
	/* Emergency charging */
	int ec_state;
	/* Monitoring */
	char notify_path[256];
	int startup;
	int tozero;
	/* Booleans */
	int have_battery_temp;
	char errmsg[128];
	int force_charge;
	JSPropertySpec *props;
};
typedef struct si_session si_session_t;

//#define SI_STATE_STARTUP	0x01
#define SI_STATE_RUNNING	0x02
#define SI_STATE_CHARGING	0x04
#define SI_STATE_OPEN		0x10

int si_can_init(si_session_t *s);
int si_smanet_init(si_session_t *s);

void *si_recv_thread(void *handle);
int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_read(si_session_t *,void *,int);
int si_get_bits(si_session_t *);
int si_can_write(si_session_t *s, int id, uint8_t *data, int data_len);
int si_write_va(si_session_t *);
int si_write(si_session_t *,void *,int);
int si_control(void *handle,char *op, char *id, json_value_t *actions);
int si_config(void *,int,...);
json_value_t *si_info(void *);
int si_config_add_info(si_session_t *s, json_value_t *j);
int si_check_params(si_session_t *s);

/* Config */
int si_read_config(si_session_t *s);
int si_write_config(si_session_t *s);
int si_set_config(si_session_t *s, char *, char *, char *);

/* Charging */
int si_check_config(si_session_t *s);
void charge_init(si_session_t *s);
void charge_max_start(si_session_t *s);
void charge_max_stop(si_session_t *s);
void charge_stop(si_session_t *s, int rep);
void charge_start(si_session_t *s, int rep);
void charge_start_cv(si_session_t *s, int rep);
void charge_check(si_session_t *s);
int charge_control(si_session_t *s, int, int);

/* SMANET funcs */
int si_start_grid(si_session_t *, int);

#define SI_VOLTAGE_MIN	36.0
#define SI_VOLTAGE_MAX	64.0

#define si_isvrange(v) ((v >= SI_VOLTAGE_MIN) && (v  <= SI_VOLTAGE_MAX))

extern solard_driver_t *si_transports[];
extern solard_driver_t si_driver;
//void si_notify(si_session_t *s,char *,...);
#define si_notify(session,format,args...) solard_notify(session->notify_path,format,## args)

#ifdef JS
int si_jsinit(si_session_t *);
#endif

#endif /* __SOLARD_SI_H */
