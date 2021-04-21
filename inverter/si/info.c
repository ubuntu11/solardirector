
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

json_value_t *si_info(void *handle) {
	si_session_t *s = handle;
	json_value_t *v;

	/* Need to do this here as info is called by agent init */
	if (load_tp_from_cfg(&s->tty,&s->tty_handle,s->ap->cfg,"config") == 0) {
		if (s->ap->cfg) {
			char *p;
			p = cfg_get_item(s->ap->cfg,"config","channels_path");
			if (p) strncat(s->channels_path,p,sizeof(s->channels_path)-1);
		}
		s->smanet = smanet_init(s->tty,s->tty_handle);
	}

	dprintf(1,"creating info...\n");

	v = json_create_object();
	if (!v) return 0;
	json_add_string(v,"agent_name","si");
	json_add_string(v,"agent_role",SOLARD_ROLE_INVERTER);
	json_add_string(v,"agent_description","Sunny Island Agent");
	json_add_string(v,"agent_version","1.0");
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
