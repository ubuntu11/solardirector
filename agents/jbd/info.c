
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"

extern char *jbd_version_string;

static int _can_get_hwinfo(jbd_session_t *s) {
	return 1;
}

static int _std_get_hwinfo(jbd_session_t *s) {
	uint8_t data[128];
	unsigned short mfgdate;
	int day,mon,year;
	float version;

	if (jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data)) < 0) return 1;
	mfgdate = jbd_getshort(&data[10]);
	day = mfgdate & 0x1f;
	mon = (mfgdate >> 5) & 0x0f;
	year = 2000 + (mfgdate >> 9);
	dprintf(3,"year: %d, mon: %d, day: %d\n", year, mon, day);
	sprintf(s->hwinfo.mfgdate,"%04d%02d%02d",year,mon,day);
	dprintf(3,"mfgdate: %s\n", s->hwinfo.mfgdate);
	version = data[18] / 10.0;
	sprintf(s->hwinfo.version,"%.1f", version);
	jbd_eeprom_start(s);
	if (jbd_rw(s, JBD_CMD_READ, JBD_REG_MFGNAME, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(s->hwinfo.manufacturer,(char *)data,sizeof(s->hwinfo.manufacturer)-1);
	dprintf(1,"mfgname: %s\n", s->hwinfo.manufacturer);
	if (jbd_rw(s, JBD_CMD_READ, JBD_REG_MODEL, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(s->hwinfo.model,(char *)data,sizeof(s->hwinfo.model)-1);
	dprintf(1,"model: %s\n", s->hwinfo.model);
	jbd_eeprom_end(s);
	return 0;
}

int jbd_get_hwinfo(jbd_session_t *s) {
	int r;

	if (!s->tp) {
		dprintf(1,"no tp!\n");
		return 1;
	}
	dprintf(5,"transport: %s\n", s->tp->name);
	if (strncmp(s->tp->name,"can",3)==0) 
		r = _can_get_hwinfo(s);
	else
		r = _std_get_hwinfo(s);
	return r;
}

int jbd_get_info(jbd_session_t *s) {
	jbd_hwinfo_t info;
	json_object_t *o;
	long mem_start;

	dprintf(1,"s: %p\n", s);
	if (!s) return 1;

	memset(&info,0,sizeof(info));
#if 0
	/* Get the info */
	if (jbd_open(s) < 0) return 1;
	if (_get_info(s,&info)) return 1;

	/* Get balance info */
	if (jbd_eeprom_start(s) < 0) return 1;
	bytes = jbd_rw(s, JBD_CMD_READ, JBD_REG_FUNCMASK, data, sizeof(data));
	if (bytes < 0) return 1;
	val = jbd_getshort(data);
	dprintf(1,"val: %d\n", val);
	if (val & JBD_FUNC_CHG_BALANCE)
		s->balancing = 2;
	else if (val & JBD_FUNC_BALANCE_EN)
		s->balancing = 1;
	else
		s->balancing = 0;
	jbd_close(s);
#endif

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

	o = json_create_object();
	if (!o) return 1;
	json_object_set_string(o,"agent_name","jbd");
	json_object_set_string(o,"agent_role",SOLARD_ROLE_BATTERY);
	json_object_set_string(o,"agent_description","JBD BMS Agent");
	json_object_set_string(o,"agent_version",jbd_version_string);
	json_object_set_string(o,"agent_author","Stephen P. Shoecraft");
	json_object_set_string(o,"device_manufacturer",info.manufacturer);
	json_object_set_string(o,"device_model",info.model);
	json_object_set_string(o,"device_mfgdate",info.mfgdate);
	json_object_set_string(o,"device_version",info.version);
#if 0
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_object_set_value(j,"controls",a);
	config_add_info(s->ap->cp,a);
	jbd_config_add_params(o);
#endif
	config_add_info(s->ap->cp,o);
	s->info = json_object_get_value(o);

	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);
	return 0;
}
