
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"

json_value_t *solard_info(void *handle) {
//	solard_config_t *s = handle;
	json_value_t *j, *a;
	long mem_start;
//	struct utsname uts;

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

//	memset(&uts,0,sizeof(uts));
//	uname(&uts);

	j = json_create_object();
	if (!j) return 0;
	json_add_string(j,"agent_name","solard");
	json_add_string(j,"agent_role","Controller");
	json_add_string(j,"agent_description","SolarD Controller");
	json_add_string(j,"agent_version","1.0");
	json_add_string(j,"agent_author","Stephen P. Shoecraft");
//	json_add_string(j,"host",uts.nodename);
//	json_add_number(j,"pid",getpid());
	a = json_create_array();
//	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_add_value(j,"controls",a);
//	solard_config_add_info(j);

	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);
	return j;
}
