
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

extern char *si_version_string;

extern solard_driver_t *si_transports[];

json_value_t *si_get_info(si_session_t *s) {
	json_object_t *o;
	char temp[256], *p;
	int i;

	dprintf(1,"creating info...\n");

	o = json_create_object();
	if (!o) return 0;
//	json_object_set_string(o,"agent_id",s->ap->m->clientid);
	json_object_set_string(o,"agent_name","si");
	json_object_set_string(o,"agent_role","Inverter");
//	agent_mktopic(temp,sizeof(temp)-1,s->ap,s->ap->instance_name,0);
//	json_object_set_string(o,"agent_topic",temp);
	json_object_set_string(o,"agent_description","SMA Sunny Island Agent");
	json_object_set_string(o,"agent_version",si_version_string);
	json_object_set_string(o,"agent_author","Stephen P. Shoecraft");
	p = temp;
	*p = 0;
	for(i=0; si_transports[i]; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p,"%s",si_transports[i]->name);
	}
	json_object_set_string(o,"agent_transports",temp);
	json_object_set_string(o,"device_manufacturer","SMA");
	if (s->smanet_connected) {
		char temp[32];

		json_object_set_string(o,"device_model",s->smanet->type);
		sprintf(temp,"%ld",s->smanet->serial);
		json_object_set_string(o,"device_serial",temp);
	}
	config_add_info(s->ap->cp, o);

	return json_object_get_value(o);
}
