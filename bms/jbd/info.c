
#include "jbd.h"

int jbd_can_get_info(jbd_session_t *s,jbd_info_t *info) {
	return 1;
}

int jbd_std_get_info(jbd_session_t *s,jbd_info_t *info) {
	uint8_t data[128];
	unsigned short mfgdate;
	int day,mon,year;

	if (jbd_rw(s, JBD_CMD_READ, JBD_CMD_HWINFO, data, sizeof(data)) < 0) return 1;
	mfgdate = jbd_getshort(&data[10]);
	day = mfgdate & 0x1f;
	mon = (mfgdate >> 5) & 0x0f;
	year = 2000 + (mfgdate >> 9);
	dprintf(3,"year: %d, mon: %d, day: %d\n", year, mon, day);
	sprintf(info->mfgdate,"%04d%02d%02d",year,mon,day);
	dprintf(3,"mfgdate: %s\n", info->mfgdate);
	info->version = data[18] / 10.0;
	jbd_eeprom_start(s);
	if (jbd_rw(s, JBD_CMD_READ, JBD_REG_MFGNAME, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(info->manufacturer,(char *)data,sizeof(info->manufacturer)-1);
	dprintf(1,"mfgname: %s\n", info->manufacturer);
	if (jbd_rw(s, JBD_CMD_READ, JBD_REG_MODEL, data, sizeof(data)) < 0) return 1;
	trim((char *)data);
	strncat(info->model,(char *)data,sizeof(info->model)-1);
	dprintf(1,"model: %s\n", info->model);
	jbd_eeprom_end(s);
	return 0;
}

int jbd_get_info(void *handle,jbd_info_t *info) {
	jbd_session_t *s = handle;
	int r;

	dprintf(5,"transport: %s\n", s->tp->name);
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jbd_can_get_info(s,info);
	else
		r = jbd_std_get_info(s,info);
	return r;
}

json_value_t *jbd_info(void *handle) {
	jbd_session_t *s = handle;
	jbd_info_t info;
	json_value_t *j,*a;
	char version[16];
	long mem_start;
	uint8_t data[4];
	uint16_t val;
	int bytes;

	dprintf(1,"s: %p\n", s);
	if (!handle) return 0;

	/* Get the info */
	if (jbd_open(s) < 0) return 0;
	memset(&info,0,sizeof(info));
	if (jbd_get_info(s,&info)) return 0;

	/* Get balance info */
	if (jbd_eeprom_start(s) < 0) return 0;
	bytes = jbd_rw(s, JBD_CMD_READ, JBD_REG_FUNCMASK, data, sizeof(data));
	if (bytes < 0) return 0;
	val = jbd_getshort(data);
	dprintf(1,"val: %d\n", val);
	if (val & JBD_FUNC_CHG_BALANCE)
		s->balancing = 2;
	else if (val & JBD_FUNC_BALANCE_EN)
		s->balancing = 1;
	else
		s->balancing = 0;
	jbd_close(s);

	mem_start = mem_used();
	dprintf(1,"mem_start: %ld\n",mem_start);

	j = json_create_object();
	if (!j) return 0;
	json_add_string(j,"agent_name","jbd");
	json_add_string(j,"agent_role",SOLARD_ROLE_BATTERY);
	json_add_string(j,"agent_description","JBD BMS Utility");
	json_add_string(j,"agent_version","1.0");
	json_add_string(j,"agent_author","Stephen P. Shoecraft");
	json_add_string(j,"device_manufacturer",info.manufacturer);
	json_add_string(j,"device_model",info.model);
	json_add_string(j,"device_mfgdate",info.mfgdate);
	sprintf(version,"%.1f",info.version);
	json_add_string(j,"device_version",version);
	a = json_create_array();
	json_array_add_descriptor(a,(json_descriptor_t){ "CHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "DISCHARGE_CONTROL", DATA_TYPE_BOOL, 0, 0, 0, 2, (char *[]){ "off", "on" } });
	json_array_add_descriptor(a,(json_descriptor_t){ "BALANCE_CONTROL", DATA_TYPE_INT, "select", 3, (int []){ 0,1,2 }, 3, (char *[]){ "off", "on", "charge" } });
	json_add_value(j,"controls",a);
	jbd_config_add_params(j);

	dprintf(1,"mem_used: %ld\n",mem_used() - mem_start);
	return j;
}
