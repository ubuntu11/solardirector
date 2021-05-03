
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

extern char *jk_version_string;

int jk_can_get_info(jk_session_t *s,jk_info_t *info) {
	return 1;
}

int jk_std_get_info(jk_session_t *s,jk_info_t *info) {
#if 0
	uint8_t data[128];
	unsigned short mfgdate;
	int day,mon,year;

	if (jk_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data)) < 0) return 1;
	mfgdate = jk_getshort(&data[10]);
	day = mfgdate & 0x1f;
	mon = (mfgdate >> 5) & 0x0f;
	year = 2000 + (mfgdate >> 9);
	dprintf(3,"year: %d, mon: %d, day: %d\n", year, mon, day);
	sprintf(info->mfgdate,"%04d%02d%02d",year,mon,day);
	dprintf(3,"mfgdate: %s\n", info->mfgdate);
	info->version = data[18] / 10.0;
	jk_eeprom_start(s);
	if (jk_rw(s, JBD_CMD_READ, JBD_REG_MFGNAME, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(info->manufacturer,(char *)data,sizeof(info->manufacturer)-1);
	dprintf(1,"mfgname: %s\n", info->manufacturer);
	if (jk_rw(s, JBD_CMD_READ, JBD_REG_MODEL, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(info->model,(char *)data,sizeof(info->model)-1);
	dprintf(1,"model: %s\n", info->model);
	jk_eeprom_end(s);
#endif
	return 1;
}

int jk_get_info(void *handle,jk_info_t *info) {
	jk_session_t *s = handle;
	int r;

	dprintf(5,"transport: %s\n", s->tp->name);
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jk_can_get_info(s,info);
	else
		r = jk_std_get_info(s,info);
	return r;
}

json_value_t *jk_info(void *handle) {
	jk_session_t *s = handle;
	jk_info_t info;
	json_value_t *j,*a;
	char version[16];
	long mem_start;
	uint8_t data[4];
	uint16_t val;
	int bytes;

	dprintf(1,"s: %p\n", s);
	if (!handle) return 0;

#if 0
	/* Get the info */
	if (jk_open(s) < 0) return 0;
	memset(&info,0,sizeof(info));
	if (jk_get_info(s,&info)) return 0;

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
	jk_close(s);
#endif

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

	j = json_create_object();
	if (!j) return 0;
	json_add_string(j,"agent_name","jk");
	json_add_string(j,"agent_role",SOLARD_ROLE_BATTERY);
	json_add_string(j,"agent_description","JIKONG BMS Agent");
	json_add_string(j,"agent_version",jk_version_string);
	json_add_string(j,"agent_author","Stephen P. Shoecraft");
//	json_add_string(j,"device_manufacturer",info.manufacturer);
//	json_add_string(j,"device_model",info.model);
//	json_add_string(j,"device_mfgdate",info.mfgdate);
//	sprintf(version,"%.1f",info.version);
//	json_add_string(j,"device_version",version);
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_add_value(j,"controls",a);
//	jk_config_add_info(j);

	{
		solard_battery_t bat;
		jk_read(s,&bat,0);
	}
	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);
	return j;
}
