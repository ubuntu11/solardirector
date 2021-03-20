
#include "solard.h"

int solard_pubinfo(solard_config_t *conf) {
	json_object_t *j;
	json_value_t *a;
	char *p;
	long mem_start;

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

	j = json_create_object();
	if (!j) return 0;
	json_add_string(j,"agent_name","jbd");
	json_add_string(j,"agent_role",AGENT_ROLE_BATTERY);
	json_add_string(j,"agent_description","JBD BMS Utility");
	json_add_string(j,"agent_version","1.0");
	json_add_string(j,"agent_author","Stephen P. Shoecraft");
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_add_array(j,"controls",a);
//	solard_config_add_params(j);
	p = json_dumps(j,0);
	json_destroy(j);

	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);

	dprintf(1,"return: %p\n", p);
	return 1;
}
