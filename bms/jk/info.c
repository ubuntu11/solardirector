
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

extern char *jk_version_string;
#define _getint(p) (int)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))

int jk_can_get_info(jk_session_t *s) {
	return 1;
}

int jk_std_get_info(jk_session_t *s) {
	return 0;
}

int jk_get_info(void *handle) {
	jk_session_t *s = handle;
	int r;

	dprintf(5,"transport: %s\n", s->tp->name);
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jk_can_get_info(s);
	else if (strcmp(s->tp->name,"bt")==0)
		r = jk_bt_read(s,0);
	else
		r = jk_std_get_info(s);
	dprintf(1,"r: %d\n", r);
	return r;
}

json_value_t *jk_info(void *handle) {
	jk_session_t *s = handle;
	json_value_t *j,*a;
	char version[16];
	long mem_start;
	uint8_t data[4];
	uint16_t val;
	int bytes;

	dprintf(1,"s: %p\n", s);
	if (!handle) return 0;

	/* Get the info */
	if (jk_open(s) < 0) return 0;
	if (jk_get_info(s)) return 0;

#if 0
	/* Get balance info */
	if (jk_eeprom_start(s) < 0) return 0;
	bytes = jk_rw(s, JBD_CMD_READ, JBD_REG_FUNCMASK, data, sizeof(data));
	if (bytes < 0) return 0;
	val = jk_getshort(data);
	dprintf(1,"val: %d\n", val);
	if (val & JBD_FUNC_CHG_BALANCE)
		s->balancing = 2;
	else if (val & JBD_FUNC_BALANCE_EN)
		s->balancing = 1;
	else
		s->balancing = 0;
#endif
	jk_close(s);

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

	j = json_create_object();
	if (!j) return 0;
	json_add_string(j,"agent_name","jk");
	json_add_string(j,"agent_role",SOLARD_ROLE_BATTERY);
	json_add_string(j,"agent_description","JIKONG BMS Agent");
	json_add_string(j,"agent_version",jk_version_string);
	json_add_string(j,"agent_author","Stephen P. Shoecraft");
	json_add_string(j,"device_manufacturer","JIKONG");
	json_add_string(j,"device_model",s->info.model);
//	sprintf(version,"%.1f",info.version);
	json_add_string(j,"device_version",s->info.hwvers);
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_add_value(j,"controls",a);
//	jk_config_add_info(j);

	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);
	return j;
}
