
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"

extern char *sb_agent_version_string;

json_value_t *sb_get_info(sb_session_t *s) {
	json_object_t *o;
	char model[64];
	char *fields;

	dprintf(1,"creating info...\n");

	if (sb_driver.open(s)) return 0;

	*model = 0;
	fields = sb_mkfields((char *[]){ "6800_08822000" }, 1);
	if (fields) {
		if (sb_request(s,SB_GETVAL,fields) == 0)
		if (sb_get_result(s,DATA_TYPE_STRING,model,sizeof(model)-1,"6800_08822000") == 0)
			dprintf(1,"model: %s\n", model);
		free(fields);
	}

	o = json_create_object();
	if (!o) return 0;
	json_object_set_string(o,"agent_name",sb_driver.name);
	json_object_set_string(o,"agent_role",SOLARD_ROLE_INVERTER);
	json_object_set_string(o,"agent_description","SMA Sunny Boy Agent");
	json_object_set_string(o,"agent_version",sb_agent_version_string);
	json_object_set_string(o,"agent_author","Stephen P. Shoecraft");
	json_object_set_string(o,"agent_transports","http,https");
	json_object_set_string(o,"device_manufacturer","SMA");
	if (*model) json_object_set_string(o,"device_model",model);
	config_add_info(s->ap->cp, o);

	return json_object_get_value(o);
}
