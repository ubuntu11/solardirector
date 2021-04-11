
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

json_value_t *si_info(void *handle) {
//	si_session_t *s = handle;
	json_value_t *v;

	v = json_create_object();
	if (!v) return 0;
	json_add_string(v,"agent_name","si");
	json_add_string(v,"agent_role",SOLARD_ROLE_INVERTER);
	json_add_string(v,"agent_description","Sunny Island Agent");
	json_add_string(v,"agent_version","1.0");
	json_add_string(v,"agent_author","Stephen P. Shoecraft");
	json_add_string(v,"device_manufacturer","SMA");
#if 0
	json_add_string(v,"device_model",info.model);
	sprintf(version,"%.1f",info.version);
	json_add_string(j,"device_version",version);
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_add_value(j,"controls",a);
#endif
	si_config_add_params(v);

	return v;
}
