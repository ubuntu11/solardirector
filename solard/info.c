
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"

extern char *sd_version_string;

json_value_t *sd_get_info(void *handle) {
	solard_instance_t *sd = handle;
	json_object_t *j;

	j = json_create_object();
	if (!j) return 0;
	json_object_set_string(j,"agent_name","solard");
	json_object_set_string(j,"agent_class","Management");
	json_object_set_string(j,"agent_role","Controller");
	json_object_set_string(j,"agent_description","SolarD Controller");
	json_object_set_string(j,"agent_version",sd_version_string);
	json_object_set_string(j,"agent_author","Stephen P. Shoecraft");

	config_add_info(sd->ap->cp, j);

	return json_object_get_value(j);
}
