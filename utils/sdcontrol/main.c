
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "client.h"
#include "uuid.h"

int usage(int r) {
	return r;
}

int main(int argc,char **argv) {
	char topic[128],*p,*target,*control,*option,*temp,*val;
	solard_client_t *s;
	int timeout,list_flag,i,len,count;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-t:#|wait time",&timeout,DATA_TYPE_INT,0,0,"10" },
		{ "-l|list options",&list_flag,DATA_TYPE_BOOL,0,0,"N" },
		OPTS_END
	};
	list lp;
#if 0
//	char *args[] = { "t2", "-d", "2", "-l" };
//	char *args[] = { "t2", "-d", "2", "-l", "Inverter/si" };
	char *args[] = { "t2", "-d", "2", "-l", "Inverter/si", "Charge" };
//	char *args[] = { "t2", "-d", "2", "-l", "Inverter/si", "Charge", "Off" };
	#define nargs (sizeof(args)/sizeof(char *))

	argv = args;
	argc = nargs;
#endif

	s = client_init(argc,argv,opts,"sdcontrol");
	if (!s) return 1;

	dprintf(1,"optind: %d\n", optind);
        argc -= optind;
        argv += optind;

	dprintf(1,"argc: %d\n",argc);
//	if (!argc) return usage(1);

	/* Arg1: target */
	/* Arg2: Control */
	/* Arg3: Option */
	target = control = option = 0;
	dprintf(1,"argc: %d\n",argc);
	if (argc > 2) option = argv[2];
	else if (argc > 1) {
//		if (!list_flag) return usage(1);
		control = argv[1];
	}
	else if (argc > 0) {
//		if (!list_flag) return usage(1);
		target = argv[0];
	}
	dprintf(1,"argc: %d\n",argc);
	dprintf(1,"list_flag: %d, target: %s, control: %s, option: %s\n", list_flag, target, control, option);

	sprintf(topic,"%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_INFO);
	if (mqtt_sub(s->m,topic)) return 0;
	sprintf(topic,"%s/%s/%s/+",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONTROL);
	if (mqtt_sub(s->m,topic)) return 0;
	sprintf(topic,"%s/%s/%s/+/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONTROL,SOLARD_ID_STATUS,s->id);
	dprintf(1,"topic: %s\n", topic);
	if (mqtt_sub(s->m,topic)) return 0;

#if 0
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

	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,temp,len);
	count = list_count(lp);
	dprintf(1,"count: %d\n", count);
	if (!count) return usage(1);

	/* get/del: single param, no = sign */
	if (strcasecmp(action,"get")==0 || strcasecmp(action,"del")==0) {
		if (count == 1) {
			list_reset(lp);
			while((p = list_get_next(lp)) != 0) {
				val = client_get_config(s,target,p,timeout,read_flag);
				if (val) printf("%s %s\n", p, val);
			}
		} else {
			char **names;
			list values;

			names = malloc(count*sizeof(char *));
			i = 0;
			list_reset(lp);
			while((p = list_get_next(lp)) != 0) names[i++] = p;
			dprintf(1,"action: %s\n", action);
			if (strcasecmp(action,"get")==0) {
				values = client_get_mconfig(s,target,count,names,30);
				if (!values) return 1;
				i = 0;
				list_reset(values);
				while((val = list_get_next(values)) != 0) {
					printf("%s %s\n", names[i++], val);
				}
#if 0
			} else {
				if (client_del_mconfig(s,count,names,30)) {
					printf("error deleting items\n");
					return 1;
				}
#endif
			}
		}
	} else if (strcasecmp(action,"set")==0 || strcasecmp(action,"add")==0) {
		client_set_config(s,target,argv[2],argv[3],15);
	} else {
		log_write(LOG_ERROR,"invalid action: %s\n", action);
		return 1;
	}
#endif

	free(s);
	return 0;
}
