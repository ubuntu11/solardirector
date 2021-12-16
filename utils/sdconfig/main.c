
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "client.h"
#include "uuid.h"

#define TESTING 0

struct client_agent_info {
	char id[SOLARD_ID_LEN];
	char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN];
	int status;
	char errmsg[1024];
};
typedef struct client_agent_info client_agent_info_t;

static void _doconfig2(solard_client_t *s, char *message, client_agent_info_t *infop, int timeout, int read_flag) {
	char topic[SOLARD_TOPIC_LEN];
	solard_message_t *msg;
	json_value_t *v;
	int status,found;
	char *errmsg;

	dprintf(1,"id: %s, timeout: %d, read_flag: %d\n", infop->id, timeout, read_flag);

	sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,infop->id);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(s->m,topic,message,1,0);
	/* If timeout, wait for a response */
	status = 1;
	errmsg = "error";
	found = 0;
	if (timeout > 0) {
		while(timeout--) {
			dprintf(1,"count: %d\n", list_count(s->mq));
			list_reset(s->mq);
			while((msg = list_get_next(s->mq)) != 0) {
				/* If the message role is my ID, its a reply */
				solard_message_dump(msg,1);
				dprintf(1,"msg->id: %s, clientid: %s\n", msg->id, s->mqtt_config.clientid);
				if (strcmp(msg->id,s->mqtt_config.clientid) == 0) {
					dprintf(1,"Got response:\n");
					v = json_parse(msg->data);
					dprintf(1,"v: %p\n", v);
					if (v && v->type == JSONObject) {
						status = json_object_dotget_number(json_object(v),"status");
						errmsg = json_object_dotget_string(json_object(v),"message");
						dprintf(1,"status: %d, errmsg: %s\n", status, errmsg);
						found = 1;
						break;
					}
				} else {
					printf("%s: %s\n", infop->target, msg->data);
				}
				list_delete(s->mq,msg);
			}
			if (!found) sleep(1);
			else break;
		}
	}
	if (found) {
		printf("%s: %s\n", infop->target, errmsg);
	} else {
		printf("%s: no reply\n", infop->target);
	}
}

static json_value_t *catargs(char *action,int argc,char *argv[],int act) { 
	json_value_t *v,*a;
	int i;
//	list lp;
//	char *p;

	v = json_create_object();
	a = json_create_array();
	dprintf(1,"argc: %d\n", argc);
	if (act == 1) {
		for(i=2; i < argc; i++) {
			json_array_add_string(a,argv[i]);
		}
#if 0
//			dprintf(1,"adding: %s\n", argv[i]);
			conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,argv[i],strlen(argv[i]));
			list_reset(lp);
			while((p = list_get_next(lp)) !=0) {
				dprintf(1,"adding to array: %s\n", p);
				json_array_add_string(a,p);
			}
			list_destroy(lp);
		}
#endif
	} else if (act == 2) {
		json_value_t *o;
		char *label;
		int next;

		next = 0;
		o = json_create_object();
		for(i=2; i < argc; i++) {
			if (next) {
				dprintf(1,"adding: %s:%s\n",label,argv[1]);
				json_add_string(o,label,argv[i]);
				json_array_add_value(a,o);
				next = 0;
			} else {
				label = argv[i];
				next = 1;
			}
		}
		if (next) {
			log_warning("%s: no value provided, ignoring\n",label);
		}
	}
	json_add_value(v,action,a);
//	printf("json: %s\n", json_dumps(v,1));

	return v;
}

#if 0
static void _doconfig(solard_client_t *s, list lp, char *op, char *target, int timeout, int read_flag) {
	register char *p;
	char topic[SOLARD_TOPIC_LEN];
	solard_message_t *msg;

	dprintf(1,"target: %s, op: %s, timeout: %d\n", target, op, timeout);

	list_reset(lp);
	while((p = list_get_next(lp)) != 0) {
//		val = client_get_config(s,op,target,p,timeout,read_flag);
		sprintf(topic,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op);
		dprintf(1,"topic: %s\n",topic);
		mqtt_pub(s->m,topic,p,1,0);
		/* If timeout, wait for a response */
		if (timeout > 0) {
			while(timeout--) {
				list_reset(s->mq);
				while((msg = list_get_next(s->mq)) != 0) {
					solard_message_dump(msg,1);
					list_delete(s->mq,msg);
				}
				sleep(1);
			}
		}
//		if (val) printf("%s %s\n", p, val);
	}
}


int get_status(char *target, list mq) {
	solard_message_t *msg;
	int found;

	found = 0;
	while(!found) {
		list_reset(mq);
		while((msg = list_get_next(mq)) != 0) {
			dprintf(1,"msg: role: %s, name: %s, func: %s\n", msg->role, msg->name, msg->func);
			if (strcmp(msg->func,"Status")==0)
				return 0;
		}
		solard_ingest(mq, 1);
	}
	return 1;
}
#endif

int dox(void *ctx, char *label, char *value) {
	dprintf(1,"ctx: %p, label: %s, value: %s\n", label, value);
	return 0;
}

#define NAME_FORMAT "%-20.20s  "
#define TYPE_FORMAT "%-10.10s  "
#define SCOPE_FORMAT "%-10.10s  "
#define VALUE_FORMAT "%s  "
#define LABEL_FORMAT "%s"

static int do_list_item(char *target, int level, json_object_t *o, int hdr) {
	char *type,*scope;
	char pad[16],line[1024],*p;
	int i,j;

	pad[0] = 0;
	for(i=0; i < level; i++) strcat(pad,"  ");

	if (hdr) printf("%s"NAME_FORMAT""TYPE_FORMAT""SCOPE_FORMAT""VALUE_FORMAT""LABEL_FORMAT"\n",pad,"Name","Type","Scope","Values","(Labels)");
//	dprintf(1,"o->count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		dprintf(1,"[%d]: %s\n", i, o->names[i]);
		type = json_object_dotget_string(json_object(o->values[i]),"type");
		if (!type) continue;
		dprintf(1,"type: %s\n", type);
		p = line;
		p += sprintf(p,"%s"NAME_FORMAT""TYPE_FORMAT,pad,o->names[i],type);
		scope = json_object_dotget_string(json_object(o->values[i]),"scope");
		if (scope) { 
			dprintf(1,"scope: %s\n", scope);
			p += sprintf(p,SCOPE_FORMAT,scope);
			if (strcmp(scope,"select")==0) {
				json_array_t *values,*labels;
				float num;
				int inum;

				values = json_object_dotget_array(json_object(o->values[i]),"values");
				if (values) {
					for(j=0; j < values->count; j++) {
						if (j) p += sprintf(p,",");
						switch(values->items[j]->type) {
						case JSONString:
							p += sprintf(p,"%s",json_string(values->items[j]));
							break;
						case JSONNumber:
							num = json_number(values->items[j]);
							inum = num;
							if (inum == num) p += sprintf(p,"%d",inum);
							else p += sprintf(p,"%f",num);
							break;
						default:
							p += sprintf(p,"unhandled type: %d",values->items[j]->type);
							break;
						}
					}
				}
				labels = json_object_dotget_array(json_object(o->values[i]),"labels");
				dprintf(1,"labels: %p\n", labels);
				if (labels) {
					p += sprintf(p,"(");
					for(j=0; j < labels->count; j++) {
						if (j) p += sprintf(p,",");
						p += sprintf(p,"%s",json_string(labels->items[j]));
					}
					p += sprintf(p,")");
				}
			} else if (strcmp(scope,"range")==0) {
				double min,max,step;
				int imin,imax,istep;
				char *label;

				min = json_object_dotget_number(json_object(o->values[i]),"values.min");
				imin = min;
#define DTEST(a,b) !((a>b)||(a<b))
				if (imin == min || DTEST(min,0.0)) p += sprintf(p,"min: %d, ",imin);
				else p += sprintf(p,"min: %lf, ",min);
				max = json_object_dotget_number(json_object(o->values[i]),"values.max");
				imax = max;
				if (imax == max || DTEST(max,0.0)) p += sprintf(p,"max: %d, ",imax);
				else p += sprintf(p,"max: %lf, ",max);
				step = json_object_dotget_number(json_object(o->values[i]),"values.step");
				istep = step;
				if (istep == step || DTEST(step,0.0)) p += sprintf(p,"step: %d",istep);
				else p += sprintf(p,"step: %lf",step);

				label = json_object_dotget_string(json_object(o->values[i]),"label");
				if (label) p += sprintf(p," (%s)",label);
			}
		} else {
			p += sprintf(p,SCOPE_FORMAT,type);
			if (strcmp(type,"bool")==0) p += sprintf(p,"true/false");
		}
		printf("%s\n",line);
	}
	return 0;
}

static int do_list_section(char *target, json_object_t *o) {
	json_array_t *a;
	int i;

	/* A section is a single object with array of items */
	printf("  %s\n", o->names[0]);
	a = json_array(o->values[0]);
	dprintf(1,"a->count: %d\n", a->count);
	for(i = 0; i < a->count; i++) if (do_list_item(target,2,json_object(a->items[i]),i == 0)) return 1;
	return 0;
}

void do_list(solard_message_t *msg) {
	json_value_t *v;
	json_object_t *o,*o2;
	json_array_t *a,*a2;
	char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN];
	int i,j,k,sec;

	sprintf(target,"%s",msg->name);
	printf("%s:\n",target);
	v = json_parse(msg->data);
	if (!v) {
		log_error("%s: invalid json data\n",target);
		return;
	}
	o = json_object(v);
	for(i=0; i < o->count; i++) {
		dprintf(1,"label[%d]: %s\n",i,o->names[i]);
		if (strcmp(o->names[i],"configuration")!=0) continue;
		/* Configuration is an array of objects */
		if (o->values[i]->type != JSONArray) {
			printf("  invalid info format(configuration section is not an array)\n");
			return;
		}
		/* Check array for sections */
		sec = 0;
		a = json_array(o->values[i]);
		dprintf(1,"a->count: %d\n", a->count);
		for(j=0; j < a->count; j++) {
			/* Better be an object */
			dprintf(1,"[%d]: type: %d(%s)\n", j, a->items[j]->type, json_typestr(a->items[j]->type));
			if (a->items[j]->type != JSONObject) {
				printf("  invalid info format (configuration not array of objects)\n");
				return;
			}
			/* if this object has 1 entry and it's an array, it's a section */
			o2 = json_object(a->items[j]);
			dprintf(1,"o2->count: %d, values[0]->type: %d(%s)\n", o2->count, o2->values[0]->type, json_typestr(o2->values[0]->type));
			if (o2->count == 1 && o2->values[0]->type == JSONArray) sec = 1;
		}
		/* If 1 entry is a section, they must all be sections */
		if (sec) {
			for(j=0; j < a->count; j++) {
				o2 = json_object(a->items[j]);
				if (o2->count != 1 || o2->values[0]->type != JSONArray) {
					printf("  invalid info format (mixed sections and non sections)\n");
					return;
				}
				/* All array values must be objects */
				a2 = json_array(o2->values[0]);
//				dprintf(1,"a2->count: %d\n", a2->count);
				for(k=0; k < a2->count; k++) {
					if (a2->items[k]->type != JSONObject) {
						printf("  invalid info format (configuration item not an object)\n");
						return;
					}
				}
			}
		}

		for(j=0; j < a->count; j++) {
			o2 = json_object(a->items[j]);
			if (sec) {
				do_list_section(target,o2);
			} else {
				do_list_item(target,1,o2,j==0);
			}
		}
	}
}

void usage() {
	printf("usage: sdconfig <target> <Get|Set|Add|Del> <item> [<value>]\n");
	exit(1);
}

int main(int argc,char **argv) {
	char message[8192];
	char topic[128],*p,*target,*action;
	solard_client_t *s;
	solard_message_t *msg;
	int timeout,read_flag,list_flag,r;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-t:#|wait time",&timeout,DATA_TYPE_INT,0,0,"10" },
		{ "-r|read from dev not server",&read_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-l|list parameters",&list_flag,DATA_TYPE_BOOL,0,0,"0" },
		OPTS_END
	};
	list agents;
	json_value_t *v;
	client_agent_info_t info,*infop;
	time_t start,now;
#if TESTING
	char *args[] = { "sdconfig", "-d", "6", "-l", "Battery/jbd" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	s = client_init(argc,argv,opts,"sdconfig");
	if (!s) return 1;

        argc -= optind;
        argv += optind;
        optind = 0;

	dprintf(1,"argc: %d\n", argc);
	if (argc < 1) usage();
	if (argc < 2 && !list_flag) usage();

	agents = list_create();
	if (!agents) {
		log_syserror("list_create");
		return 1;
	}

	/* Arg1: target */
	/* Arg2: action */
	/* Arg3+: item(s)/values(s) */
	target = argv[0];
	action = argv[1];
	dprintf(1,"target: %s, action: %s, %d\n", target, action, argc);
	if (list_flag)
		r = 0;
	else if (strcmp(action,"Get") == 0 || strcmp(action,"Del") == 0)
		r = 1;
	else if (strcmp(action,"Set") == 0 || strcmp(action,"Add") == 0)
		r = 2;
	dprintf(1,"argc: %d, r: %d\n", argc, r);
//	if ((r == 1 && argc < 3) || (r != 1 && argc-2 != r)) usage();
	if (r && argc-2 < r) usage();

	/* Send a "ping" request */
	sprintf(topic,"%s/%s/Ping",SOLARD_TOPIC_ROOT,target);
	mqtt_pub(s->m,topic,s->mqtt_config.clientid,1,0);

	/* Get the replie(s) */
	time(&start);
	while(1) {
		list_reset(s->mq);
		while((msg = list_get_next(s->mq)) != 0) {
			solard_message_dump(msg,1);
			dprintf(1,"id: %s, clientid: %s\n", msg->id,s->mqtt_config.clientid);
			if (strcmp(msg->id,s->mqtt_config.clientid)!=0) {
				list_delete(s->mq,msg);
				continue;
			}
			memset(&info,0,sizeof(info));
			v = json_parse(msg->data);
			if (!v) {
				dprintf(1,"%s: not a valid json struct\n",msg->data);
				continue;
			}
			p = json_object_dotget_string(json_object(v),"Pong.ID");
			if (p) {
				strcpy(info.id,p);
				p = json_object_dotget_string(json_object(v),"Pong.Target");
				if (p) strcpy(info.target,p);
			}
			json_destroy(v);
			if (!strlen(info.target)) continue;
			dprintf(1,"adding: id: %s, target: %s\n",info.id,info.target);
			list_add(agents,&info,sizeof(info));
			list_delete(s->mq,msg);
		}
		time(&now);
		if ((now - start) > 1)  break;
		sleep(1);
	}
	if (!list_count(agents)) {
		log_error("no agents responded\n");
		return 1;
	}

	if (list_flag) {
		/* Sub to the target(s) info */
		list_reset(agents);
		while((infop = list_get_next(agents)) != 0) {
			sprintf(topic,"%s/%s/%s",SOLARD_TOPIC_ROOT,infop->target,SOLARD_FUNC_INFO);
			if (mqtt_sub(s->m,topic)) return 0;
		}
		solard_ingest(s->mq, 1);
		list_reset(s->mq);
		while((msg = list_get_next(s->mq)) != 0) do_list(msg);
		return 0;
	}

	/* Sub to the target(s) ID and config */
	list_reset(agents);
	while((infop = list_get_next(agents)) != 0) {
		sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,infop->id);
		dprintf(1,"topic: %s\n", topic);
		if (mqtt_sub(s->m,topic)) return 0;
		sprintf(topic,"%s/%s/%s/+",SOLARD_TOPIC_ROOT,infop->target,SOLARD_FUNC_CONFIG);
		dprintf(1,"topic: %s\n", topic);
		if (mqtt_sub(s->m,topic)) return 0;
	}

	/* Ingest the configs */
	solard_ingest(s->mq, 1);

	dprintf(1,"count: %d\n", list_count(s->mq));
	list_reset(s->mq);
	while((msg = list_get_next(s->mq)) != 0) {
		dprintf(1,"msg->replyto: %p\n",msg->replyto);
		dprintf(1,"Got response:\n");
		solard_message_dump(msg,1);
		v = json_parse(msg->data);
		dprintf(1,"v: %p\n", v);
	}

	v = catargs(action,argc,argv,r);
	if (!v) return 1;
	json_dumps_r(v,message,sizeof(message)-1);
	dprintf(1,"message: %s\n", message);
	list_reset(agents);
	if (strcmp(action,"Add") == 0) timeout += 30;
	while((infop = list_get_next(agents)) != 0) {
		_doconfig2(s,message,infop,timeout,read_flag);
	}
	json_destroy(v);

#if 0
	{
		json_value_t *v,*a;
		list l;

		len = 0;
		for(i=2; i < argc; i++) len += strlen(argv[i])+1;
		/* Alloc a string to hold */
		temp = malloc(len+1);
		if (!temp) {
			log_write(LOG_SYSERR,"malloc temp(%d)",len+1);
			return 1;
		}
		p = temp;
		for(i=2; i < argc; i++) p += sprintf(p,"%s ",argv[i]);
		dprintf(1,"temp: %s\n", temp);
		conv_type(DATA_TYPE_LIST,&l,0,DATA_TYPE_STRING,temp,strlen(temp));
		free(temp);

	v = json_create_object();
	a = json_create_array();
	list_reset(l);
	while((p = list_get_next(l)) != 0) json_array_add_string(a,p);
	json_add_value(v,action,a);
	json_dumps_r(v,message,sizeof(message)-1);
	dprintf(1,"message: %s\n", message);
	list_reset(agents);
	while((infop = list_get_next(agents)) != 0) {
		_doconfig2(s,message,infop->id,timeout,read_flag);
	}
	json_destroy(v);
	}
#endif
	return 0;

#if 0
	/* Get/Set/Add/Del */
	dprintf(1,"action: %s\n", action);
	if (strcasecmp(action,"get") == 0 || strcasecmp(action,"read") == 0) {
		/* All args are items to get */
		v = catargs("Get",argc,argv);
		json_tostring(v,message,sizeof(message)-1,0);
	} else if (strcasecmp(action,"del") == 0 || strcasecmp(action,"delete") == 0 || strcasecmp(action,"remove") == 0) {
		/* All args are items to delete */
		v = catargs("Del",argc,argv);
		json_tostring(v,message,sizeof(message)-1,0);
	} else if (strcasecmp(action,"set") == 0 || strcasecmp(action,"write") == 0) {
		/* All args are label/value pairs with equal sign */
		/* Compile all arguments into a single string */
		/* Get the length */
		len = 0;
		for(i=2; i < argc; i++) len += strlen(argv[i])+1;
		/* Alloc a string to hold */
		temp = malloc(len+1);
		if (!temp) {
			log_write(LOG_SYSERR,"malloc temp(%d)",len+1);
			return 1;
		}
		p = temp;
		for(i=2; i < argc; i++) p += sprintf(p,"%s ",argv[i]);
		dprintf(1,"temp: %s\n", temp);
		strncpy(message,temp,sizeof(message)-1);
		free(temp);
	}
	dprintf(1,"message: %s\n", message);
	_doconfig2(s,message,target,timeout,read_flag);
	exit(0);

#if 0
	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,temp,len);
	count = list_count(lp);
	dprintf(1,"count: %d\n", count);
	if (!count) return usage(1);
#endif

	if (strcasecmp(action,"get") == 0 || strcasecmp(action,"del") == 0) {
		/* get/del: single param, no = sign */
		if (count == 1) {
			if (strcasecmp(action,"get")==0) {
				_doconfig(s,lp,"Get",target,timeout,read_flag);
			} else if (strcasecmp(action,"del")==0) {
				_doconfig(s,lp,"Del",target,timeout,read_flag);
			}
#if 0
		} else {
			char **names;
			list values;

			names = malloc(count*sizeof(char *));
			i = 0;
			list_reset(lp);
			while((p = list_get_next(lp)) != 0) names[i++] = p;
			dprintf(1,"action: %s\n", action);
			if (strcasecmp(action,"get")==0) {
				values = client_get_mconfig(s,"Get",target,count,names,30);
				if (!values) return 1;
				i = 0;
				list_reset(values);
				while((val = list_get_next(values)) != 0) {
					printf("%s %s\n", names[i++], val);
				}
#if 0
			} else if (strcasecmp(action,"del")==0) {
				if (client_del_mconfig(s,count,names,30)) {
					printf("error deleting items\n");
					return 1;
				}
#endif
			}
#endif
		}
#if 0
	} else if (strcasecmp(action,"set")==0) {
		client_set_config(s,"Set",target,argv[2],argv[3],15);
#endif
	} else if (strcasecmp(action,"add")==0) {
//		client_set_config(s,"Add",target,argv[2],argv[3],30);
		_doconfig(s,lp,"Add",target,timeout,read_flag);
	} else {
		log_write(LOG_ERROR,"invalid action: %s\n", action);
		return 1;
	}

	free(s);
#endif
	return 0;
}
