
#include "agent.h"

struct agent_config_item {
	char name;			/* Item name */
	int type;			/* Item type */
	void *ptr;			/* Pointer to data */
	int len;			/* Length of data */
};

/* Agent config functions */
int agent_config_add(solard_agent_t *ap,char *label, char *value) {
	return 1;
}

int agent_config_del(solard_agent_t *ap,char *label, char *value) {
	return 1;
}

int agent_config_get(solard_agent_t *ap,char *label, char *value) {
	return 1;
}

int agent_config_set(solard_agent_t *ap,char *label, char *value) {
	return 1;
}

int agent_config_pub(solard_agent_t *ap) {
	char topic[SOLARD_TOPIC_LEN],*s;
	json_value_t *v;

	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,SOLARD_FUNC_CONFIG);

	s = malloc(MQTT_MAX_MESSAGE_SIZE);
	if (!s) return 1;

	v = json_create_object();
	
	json_dumps_r(v,s,MQTT_MAX_MESSAGE_SIZE);
	json_destroy(v);

	return mqtt_pub(ap->m,topic,s,1,1);
}

int agent_config_process(solard_message_t *msg,agent_config_process_callback_t *func,void *ctx,char *errmsg) {
	json_value_t *v,*vv,*vvv;
	json_object_t *o;
	json_array_t *a;
	int i,j,status;
	char item[4096];
	char *p,*label,*value,*action;

	dprintf(1,"dumping message...\n");
	solard_message_dump(msg,1);

	status = 1;
	dprintf(1,"parsing json data...\n");
	v = json_parse(msg->data);
	if (!v) {
		strcpy(errmsg,"invalid json format");
		goto process_error;
	}

	/* Format is: {"action":["x=y"]} */
	dprintf(1,"type: %d (%s)\n", v->type, json_typestr(v->type));
	if (v->type != JSONObject) {
		strcpy(errmsg,"invalid json format");
		goto process_error;
	}
	o = v->value.object;
	dprintf(1,"object count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		action = o->names[i];
		dprintf(1,"action: %s\n", action);

		/* Value MUST be array */
		vv = o->values[i];
		dprintf(1,"value[%d]: type: %d (%s)\n", i, vv->type, json_typestr(vv->type));
		if (vv->type != JSONArray) {
			strcpy(errmsg,"invalid json format");
			goto process_error;
		}

		/* Array values can be any json supported format */
		a = vv->value.array;
		dprintf(1,"array count: %d\n", a->count);
		for(j=0; j < a->count; j++) {
			vvv = a->items[j];
			dprintf(1,"array[%d]: type: %d (%s)\n", j, vvv->type, json_typestr(vvv->type));
			json_conv_value(DATA_TYPE_STRING,item,sizeof(item)-1,vvv);
			dprintf(1,"item: %s\n", item);
			p = strchr(item,'=');
			if (p) {
				*p = 0;
				value = p+1;
				label = item;
			} else {
				label = item;
				value = 0;
			}
			if (func(ctx,action,label,value,errmsg)) goto process_error;
		}
	}

	status = 0;
	strcpy(errmsg,"success");

process_error:
	json_destroy(v);
	return status;
}
