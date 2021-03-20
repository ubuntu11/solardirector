
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "cellmon.h"
#include "parson.h"
#include <sys/types.h>
#include <pwd.h>

solard_battery_t *find_pack_by_uuid(cellmon_config_t *conf, char *uuid) {
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

solard_battery_t *find_pack_by_name(cellmon_config_t *conf, char *name) {
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

void dump_pack(solard_battery_t *pp) {
	register int i;

//	dprintf(1,"%s: uuid: %s",pp->name,pp->uuid);
	dprintf(1,"%s: capacity: %3.3f\n",pp->name,pp->capacity);
	dprintf(1,"%s: voltage: %3.3f\n",pp->name,pp->voltage);
	dprintf(1,"%s: current: %3.3f\n",pp->name,pp->current);
	dprintf(1,"%s: status: %d\n",pp->name,pp->errcode);
	dprintf(1,"%s: ntemps: %d\n",pp->name,pp->ntemps);
	for(i=0; i < pp->ntemps; i++) dprintf(1,"%s: temps[%d]: %3.3f\n",pp->name,i,pp->temps[i]);
	dprintf(1,"%s: cells: %d\n",pp->name,pp->ncells);
	for(i=0; i < pp->ncells; i++) dprintf(1,"%s: cellvolt[%d]: %3.3f\n",pp->name,pp->cellvolt[i]);
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
#if 0
	i=0;
	while(1) {
		p = strele(i++,",",str);
		if (!strlen(p)) break;
		dprintf(1,"p: %s\n", p);
		if (strcmp(p,"Charging") == 0)
			solard_set_state(pp,BATTERY_STATE_CHARGING);
		else if (strcmp(p,"Discharging") == 0)
			solard_set_state(pp,BATTERY_STATE_DISCHARGING);
		else if (strcmp(p,"Balancing") == 0)
			solard_set_state(pp,BATTERY_STATE_BALANCING);
	}
#endif
}

#if 0
void get_balance(solard_battery_t *pp, char *str) {
	int i,len;
	unsigned long mask;

	dprintf(1,"str: %s\n", str);

	mask = 1;
	len = strlen(str);
	for(i=0; i < len; i++) {
		if (str[i] == '1') pp->balancebits |= mask;
		mask <<= 1;
	}
	dprintf(1,"%04x\n", pp->balancebits);
}
#endif

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

void add_or_update_pack(cellmon_config_t *conf, char *name, char *payload) {
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
			else if (strcasecmp(label,"Status")==0)
				pp->errcode = num;
			else if (strcasecmp(label,"cell_min")==0 || strcasecmp(label,"cellmin")==0)
				pp->cell_min = num;
			else if (strcasecmp(label,"cell_max")==0 || strcasecmp(label,"cellmax")==0)
				pp->cell_max = num;
			else if (strcasecmp(label,"cell_diff")==0 || strcasecmp(label,"celldiff")==0)
				pp->cell_diff = num;
			else if (strcasecmp(label,"cell_avg")==0 || strcasecmp(label,"cellavg")==0)
				pp->cell_avg = num;
			else if (strcasecmp(label,"cell_total")==0 || strcasecmp(label,"celltotal")==0)
				pp->cell_total = num;
#if 0
 "CellTotal": 52.35200500488281,
  "CellMin": 3.7019999027252197,
  "CellMax": 3.7739999294281006,
  "CellDiff": 0.07200002670288086,
  "CellAvg": 3.739428997039795,
#endif
			break;
		case JSONArray:
			if (strcmp(label,"Temps")==0 || strcmp(label,"temps")==0)
				get_temps(pp,json_value_get_array(value));
			else if (strcmp(label,"cellvolt")==0 || strcmp(label,"Cells")==0)
				get_cellvolts(pp,json_value_get_array(value));
#if 0
			else if (strcmp(label,"cellres")==0)
				get_cellres(pp,json_value_get_array(value));
#endif
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

#if 0
int mycb(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	int len;
	char topic[128],payload[1024],name[32];

	dprintf(1,"ctx: %p, topicName: %p, topicLen: %d, message: %p\n", ctx, topicName, topicLen, message);
	if (topicLen) {
		len = topicLen > sizeof(topic)-1 ? sizeof(topic)-1 : topicLen;
		memcpy(topic,topicName,len);
		topic[len] = 0;
	} else {
		topic[0] = 0;
		strncat(topic,topicName,sizeof(topic)-1);
	}
	dprintf(1,"topic: %s\n",topic);
	len = message->payloadlen > sizeof(payload)-1 ? sizeof(payload)-1 : message->payloadlen;
	memcpy(payload,message->payload,len);
        payload[len] = 0;
	dprintf(5,"payload: %s\n", payload);
	/* /SolarD/Battery/<name>/Data */
	name[0] = 0;
	strncat(name,strele(3,"/",topic),sizeof(name)-1);
	dprintf(1,"name: %s\n", name);
	add_or_update_pack(ctx,name,payload);
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topic);
	dprintf(1,"returning...\n");
	return TRUE;
}

	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>	debug output\n");
#endif
	printf("  -h		this output\n");
}

#if 0
int main(int argc, char **argv) {
	int opt;
	char type[MYBMM_MODULE_NAME_LEN+1],transport[MYBMM_MODULE_NAME_LEN+1],target[MYBMM_TARGET_LEN+1],*opts;
	cellmon_config_t *conf;
	mqtt_session_t *m;

	opts = 0;
	type[0] = transport[0] = target[0] = 0;
	while ((opt=getopt(argc, argv, "d:i:h")) != -1) {
		switch (opt) {
		case 'd':
			debug=atoi(optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
	}

	/* Create the config */
	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	dprintf(2,"type: %s\n", type);
	if (strlen(type)) {
		/* Single pack */
		mybmm_module_t *cp,*tp;
		solard_battery_t pack;

		tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
		if (!tp) return 1;
		cp = mybmm_load_module(conf,type,MYBMM_MODTYPE_CELLMON);
		if (!cp) return 1;
		if (init_pack(&pack,conf,type,transport,target,opts,cp,tp)) return 1;
	} else {
		/* Read from config */
		conf->filename = find_config_file("cellmon.conf");
		if (!conf->filename) {
			printf("error: unable to find cellmon.conf");
			return 1;
		}
		conf->cfg = cfg_read(conf->filename);
		dprintf(3,"cfg: %p\n", conf->cfg);
		if (!conf->cfg) {
			printf("error: unable to read config file '%s': %s\n", conf->filename, strerror(errno));
			return 1;
		}
		mqtt_init(conf);
		pack_init(conf);
	}

	if (strlen(conf->mqtt_broker)) {
		char topic[192];

		sprintf(topic,"%s/#",conf->mqtt_topic);
		m = mqtt_new(conf->mqtt_broker,"Cellmon",topic);
		if (mqtt_setcb(m,conf,0,mycb,0)) return 1;
		if (mqtt_connect(m,20,conf->mqtt_username,conf->mqtt_password)) return 1;
		if (mqtt_sub(m,topic)) return 1;
	}

	/* main loop */
	conf->cell_crit_high = 9.9;
	while(1) {
		display(conf);
		if (debug) {
			conf->state = 0;
			while(!conf->state) sleep(1);
		} else {
			sleep(1);
		}
	}

	return 0;
}
#endif
#endif

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
//	char *args[] = { "t2", "-d", "2", "-c", "cellmon.conf" };
//	#define nargs (sizeof(args)/sizeof(char *))
	cellmon_config_t *conf;
	solard_message_t *msg;
	char *configfile;
	long start;
	int web_flag;
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-w|web output",&web_flag,DATA_TYPE_BOOL,0,0,"false" },
		OPTS_END
	};
//	time_t last_read,cur,diff;

	configfile = find_config_file("cellmon.conf");

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;
//	conf->c = client_init(nargs,args,opts,"cellmon");
	conf->c = client_init(argc,argv,opts,"cellmon",configfile);
	if (!conf->c) return 1;
	conf->packs = list_create();

	if (mqtt_sub(conf->c->m,"/SolarD/Battery/+/Data")) return 1;

	/* main loop */
	start = mem_used();
//	time(&last_read);
	while(1) {
		dprintf(1,"count: %d\n", list_count(conf->c->messages));
		list_reset(conf->c->messages);
		while((msg = list_get_next(conf->c->messages)) != 0) {
			add_or_update_pack(conf,msg->name,msg->data);
			list_delete(conf->c->messages,msg);
		}
#if 0
		if (web_flag) {
			/* Call read func */
			time(&cur);
			diff = cur - last_read;
			dprintf(3,"diff: %d, read_interval: %d\n", (int)diff, 30);
			if (diff >= 30) {
				web_display(conf,web_flag);
				break;
			}
		} else
#endif
		display(conf);
		dprintf(1,"used: %ld\n", mem_used() - start);
#if 0
		if (debug) {
			conf->state = 0;
			while(!conf->state) sleep(1);
		} else {
			sleep(1);
		}
#endif
		sleep(1);
	}
	return 0;
}
