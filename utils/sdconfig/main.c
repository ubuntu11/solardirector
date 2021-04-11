
#include "client.h"
#include "uuid.h"

int usage(int r) {
	return r;
}

int main(int argc,char **argv) {
	char topic[128],*p,*target,*action,*temp,*val;
	solard_client_t *s;
//	char *args[] = { "t2", "-d", "0", "-c", "sdconfig.conf" };
//	#define nargs (sizeof(args)/sizeof(char *))
	int timeout,read_flag,i,len,count;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-t:#|wait time",&timeout,DATA_TYPE_INT,0,0,"10" },
		{ "-r|read from dev not server",&read_flag,DATA_TYPE_BOOL,0,0,"0" },
		OPTS_END
	};
	list lp;

//	s = client_init(nargs,args,0,"sdconfig","sdconfig.conf");
	s = client_init(argc,argv,opts,"sdconfig","sdconfig.conf");
	if (!s) return 1;

//	optind -= (nargs-1);
        argc -= optind;
        argv += optind;
        optind = 0;

	dprintf(1,"argc: %d\n",argc);
	if (argc < 3) return usage(1);

	/* Arg1: target */
	/* Arg2: action */
	/* Arg3+: item(s)/values(s) */
	target = argv[0];
	action = argv[1];
	dprintf(1,"target: %s, action: %s\n", target, action);

	sprintf(topic,"%s/%s/%s/+",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG);
	if (mqtt_sub(s->m,topic)) return 0;
	sprintf(topic,"%s/%s/%s/+/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ID_STATUS,s->id);
	dprintf(1,"topic: %s\n", topic);
	if (mqtt_sub(s->m,topic)) return 0;

	/* Compile all arguments into a single string */
	/* Get the length */
	for(i=2; i < argc; i++) len += strlen(argv[i])+1;
	/* Alloc a string to hold */
	temp = malloc(len+1);
	if (!temp) {
		log_write(LOG_SYSERR,"malloc temp");
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

	free(s);
	return 0;
}
