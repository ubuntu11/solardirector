
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

extern char *si_agent_version_string;

json_value_t *si_info(void *handle) {
	si_session_t *s = handle;
	json_value_t *v;
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	cfg_proctab_t ttytab[] = {
		{ s->ap->section_name, "smanet_transport", 0, DATA_TYPE_STRING, &transport, sizeof(transport)-1,"" },
		{ s->ap->section_name, "smanet_target", 0, DATA_TYPE_STRING, &target, sizeof(target)-1,"" },
		{ s->ap->section_name, "smanet_topts", 0, DATA_TYPE_STRING, &topts, sizeof(topts)-1,"" },
		{ s->ap->section_name, "smanet_channels_path", 0, DATA_TYPE_STRING, &s->channels_path, sizeof(s->channels_path)-1,"" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,ttytab);
	if (debug) cfg_disp_tab(ttytab,"tty",0);
	if (strlen(transport) && strlen(target)) {
		solard_module_t *tp;

		tp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
		if (tp) {
			void *tp_handle;

			tp_handle = tp->new(s->ap, target, topts);
			if (tp_handle) {
				s->smanet = smanet_init(tp,tp_handle);
			} else {
				log_write(LOG_ERROR,"error creating new %s transport instance", transport);
			}
		} else {
			log_write(LOG_ERROR,"unable to load tty transport: %s",transport);
		}
	} else {
		log_write(LOG_INFO,"smanet transport or target null, skipping smanet init");
	}

#if 0
	/* Need to do this here as info is called by agent init */
	p = cfg_get_item(s->ap->cfg, s->ap->section_name, "config_transport");
	dprintf(1,"p: %s\n", p);
	if (load_tp_from_cfg(&s->tty,&s->tty_handle,s->ap->cfg,"config") == 0) {
		if (s->ap->cfg) {
			char *p;
			p = cfg_get_item(s->ap->cfg,"config","channels_path");
			if (p) strncat(s->channels_path,p,sizeof(s->channels_path)-1);
		}
		s->smanet = smanet_init(s->tty,s->tty_handle);
	}
#endif

	dprintf(1,"creating info...\n");

	v = json_create_object();
	if (!v) return 0;
	json_add_string(v,"agent_name","si");
	json_add_string(v,"agent_role",SOLARD_ROLE_INVERTER);
	json_add_string(v,"agent_description","SMA Sunny Island Agent");
	json_add_string(v,"agent_version",si_agent_version_string);
	json_add_string(v,"agent_author","Stephen P. Shoecraft");
	json_add_string(v,"device_manufacturer","SMA");
	if (s->smanet) {
		char temp[32];

		json_add_string(v,"device_model",s->smanet->type);
		sprintf(temp,"%ld",s->smanet->serial);
		json_add_string(v,"device_serial",temp);
	}
#if 0
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_add_value(j,"controls",a);
#endif
	si_config_add_info(s,v);

	return v;
}
