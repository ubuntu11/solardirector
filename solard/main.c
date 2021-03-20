
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "parson.h"
#include <sys/types.h>
#include <pwd.h>

solard_battery_t *find_pack_by_uuid(solard_config_t *conf, char *uuid) {
	solard_battery_t *pp;

	dprintf(1,"uuid: %s\n", uuid);
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (strcmp(pp->uuid,uuid) == 0) {
			dprintf(1,"found!\n");
			return pp;
		}
	}
	dprintf(1,"NOT found!\n");
	return 0;
}

solard_battery_t *find_pack_by_name(solard_config_t *conf, char *name) {
	solard_battery_t *pp;

	dprintf(1,"name: %s\n", name);
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (strcmp(pp->name,name) == 0) {
			dprintf(1,"found!\n");
			return pp;
		}
	}
	dprintf(1,"NOT found!\n");
	return 0;
}

void get_temps(solard_battery_t *pp, JSON_Array *array) {
	int i,count;
	JSON_Value *value;
	float f;

	count = json_array_get_count(array);
	for(i=0; i < count; i++) {
		value = json_array_get_value(array, i);
		f = json_value_get_number(value);
		dprintf(1,"i: %d, f: %f\n", i, f);
#define FTEMP(v) ( ( ( (float)(v) * 9.0 ) / 5.0) + 32.0)
		pp->temps[i] = FTEMP(f);
	}
	pp->ntemps = count;
}

void get_cellvolts(solard_battery_t *pp, JSON_Array *array) {
	int i,count;
	JSON_Value *value;
	float f;

	count = json_array_get_count(array);
	for(i=0; i < count; i++) {
		value = json_array_get_value(array, i);
		f = json_value_get_number(value);
		dprintf(1,"i: %d, f: %f\n", i, f);
		pp->cellvolt[i] = f;
	}
	pp->ncells = count;
}

void get_cellres(solard_battery_t *pp, JSON_Array *array) {
	int i,count;
	JSON_Value *value;
	float f;

	count = json_array_get_count(array);
	if (count != pp->ncells) {
		log_write(LOG_ERROR,"error: battery %s cellres count != cellvolt count!\n", pp->name);
	}
	for(i=0; i < count; i++) {
		value = json_array_get_value(array, i);
		f = json_value_get_number(value);
		dprintf(1,"i: %d, f: %f\n", i, f);
		pp->cellres[i] = f;
	}
//	pp->nres = count;
}

void dump_pack(solard_battery_t *pp) {
	register int i;

//	dprintf(1,"%s: uuid: %s",pp->name,pp->uuid);
	dprintf(1,"%s: capacity: %3.3f\n",pp->name,pp->capacity);
	dprintf(1,"%s: voltage: %3.3f\n",pp->name,pp->voltage);
	dprintf(1,"%s: current: %3.3f\n",pp->name,pp->current);
	dprintf(1,"%s: errcode: %d\n",pp->name,pp->errcode);
	dprintf(1,"%s: ntemps: %d\n",pp->name,pp->ntemps);
	for(i=0; i < pp->ntemps; i++) dprintf(1,"%s: temps[%d]: %3.3f\n",pp->name,i,pp->temps[i]);
	dprintf(1,"%s: cells: %d\n",pp->name,pp->ncells);
	for(i=0; i < pp->ncells; i++) dprintf(1,"%s: cellvolt[%d]: %3.3f\n",pp->name,pp->cellvolt[i]);
	for(i=0; i < pp->ncells; i++) dprintf(1,"%s: cellres[%d]: %3.3f\n",pp->name,pp->cellres[i]);
}

void get_state(solard_battery_t *pp, char *str) {
//	int i;
//	char *p;

	dprintf(1,"str: %s\n", str);

	if (strstr(str,"Charging"))
		solard_set_state(pp,BATTERY_STATE_CHARGING);
	else
		solard_clear_state(pp,BATTERY_STATE_CHARGING);

	if (strstr(str,"Discharging"))
		solard_set_state(pp,BATTERY_STATE_DISCHARGING);
	else
		solard_clear_state(pp,BATTERY_STATE_DISCHARGING);

	if (strstr(str,"Balancing"))
		solard_set_state(pp,BATTERY_STATE_BALANCING);
	else
		solard_clear_state(pp,BATTERY_STATE_BALANCING);
}

int sort_packs(void *i1, void *i2) {
	solard_battery_t *p1 = (solard_battery_t *)i1;
	solard_battery_t *p2 = (solard_battery_t *)i2;
	int val;

	dprintf(7,"p1: %s, p2: %s\n", p1->name, p2->name);
	val = strcmp(p1->name,p2->name);
	if (val < 0)
		val =  -1;
	else if (val > 0)
		val = 1;
	dprintf(7,"returning: %d\n", val);
	return val;

}

void add_or_update_pack(solard_config_t *conf, char *name, char *payload) {
	solard_battery_t pack,*pp;
	JSON_Object *object;
	JSON_Value *root,*value;
	int i,count,type,isnew;
	char *label, *valp;
	double num;

	dprintf(1,"conf: %p, name: %s, data: %s\n", conf, name, payload);

	dprintf(1,"parsing...\n");
	root = json_parse_string(payload);
	if (json_value_get_type(root) != JSONObject) {
		printf("error: not a valid json string\n");
		return;
	}
	object = json_value_get_object(root);
	count  = json_object_get_count(object);
	/* First, try to find the uuid */
	pp = 0;
	for (i = 0; i < count; i++) {
		label = (char *)json_object_get_name(object, i);
		if (!label) continue;
		if (strcmp(label,"uuid") == 0) {
			value = json_object_get_value(object, label);
			pp = find_pack_by_uuid(conf,(char *)json_value_get_string(value));
			break;
		}
	}
	if (!pp) pp = find_pack_by_name(conf,name);
	if (!pp) {
		memset(&pack,0,sizeof(pack));
		pp = &pack;
		strcpy(pack.name,name);
		isnew = 1;
	} else
		isnew = 0;
	for (i = 0; i < count; i++) {
		label = (char *)json_object_get_name(object, i);
		if (!label) {
			printf("error getting object name\n");
			return;
		}
		dprintf(3,"label: %s\n", label);
		value = json_object_get_value(object, label);
		type = json_value_get_type(value);
		switch(type) {
		case JSONString:
			valp = (char *)json_value_get_string(value);
			dprintf(1,"string: %s\n", valp);
			if (strcmp(label,"name") == 0)
				strcpy(pp->name,valp);
			else if (strcmp(label,"uuid") == 0)
				strcpy(pp->uuid,valp);
			else if (strcmp(label,"states") == 0)
				get_state(pp,valp);
//			else if (strcmp(label,"balancebits") == 0)
//				get_balance(pp,valp);
			break;
		case JSONNumber:
			num = json_value_get_number(value);
			dprintf(1,"num: %f\n", num);
			if (strcasecmp(label,"Voltage")==0)
				pp->voltage = num;
			else if (strcasecmp(label,"Current")==0)
				pp->current = num;
			else if (strcasecmp(label,"Capacity")==0)
				pp->capacity = num;
			else if (strcasecmp(label,"errcode")==0)
				pp->errcode = num;
			break;
		case JSONArray:
			if (strcmp(label,"Temps")==0 || strcmp(label,"temps")==0)
				get_temps(pp,json_value_get_array(value));
			else if (strcmp(label,"cellvolt")==0 || strcmp(label,"Cells")==0)
				get_cellvolts(pp,json_value_get_array(value));
			else if (strcmp(label,"cellres")==0)
				get_cellres(pp,json_value_get_array(value));
			else {
				printf("unknown array: %s\n",label);
			}
			break;
		default:
			printf("error: bad type in json file: %d\n", type);
			break;
		}
	}
	solard_set_state(pp,BATTERY_STATE_UPDATED);
	dprintf(1,"%s: setting last_update\n", pp->name);
	time(&pp->last_update);
	dprintf(1,"isnew: %d\n", isnew);
	if (isnew) {
//		dump_pack(pp);
		dprintf(1,"adding pack...\n");
		list_add(conf->packs,pp,sizeof(*pp));
		dprintf(1,"sorting packs...\n");
		list_sort(conf->packs,sort_packs,0);
	}
	dprintf(1,"returning...\n");
	conf->state = 1;
}

char *find_config_file(char *name) {
	static char temp[1024];
	long uid;
	struct passwd *pw;

	if (access(name,R_OK)==0) return name;
	uid = getuid();
	pw = getpwuid(uid);
	if (pw) {
		sprintf(temp,"%s/etc/%s",pw->pw_dir,name);
		dprintf(1,"temp: %s\n", temp);
		if (access(temp,R_OK)==0) return temp;
	}

	sprintf(temp,"/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/usr/local/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/opt/mybmm/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	return 0;
}

int main(int argc,char **argv) {
	char *args[] = { "t2", "-d", "2", "-c", "solard.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	solard_config_t *conf;
	solard_message_t *msg;
	char *configfile;
	long start;

	configfile = find_config_file("solard.conf");

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;
	conf->c = client_init(nargs,args,0,"solard",configfile);
//	conf->c = client_init(argc,argv,0,"solard",configfile);
	if (!conf->c) return 1;
	conf->packs = list_create();

	if (mqtt_sub(conf->c->m,"/SolarD/Battery/+/Data")) return 1;

	/* Publish our info */
	solard_pubinfo(conf);

	/* main loop */
	start = mem_used();
	while(1) {
		dprintf(1,"count: %d\n", list_count(conf->c->messages));
		list_reset(conf->c->messages);
		while((msg = list_get_next(conf->c->messages)) != 0) {
			add_or_update_pack(conf,msg->name,msg->data);
			list_delete(conf->c->messages,msg);
		}
		dprintf(1,"used: %ld\n", mem_used() - start);
		sleep(1);
	}
	return 0;
}
