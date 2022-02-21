
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sdconfig.h"

#define TESTING 0
#define dlevel 4

int main(int argc,char **argv) {
	char target[SOLARD_NAME_LEN];
	char func[SOLARD_FUNC_LEN];
	solard_client_t *c;
	client_agentinfo_t *ap;
	int timeout,no_flag,list_flag,ping_flag,read_flag,func_flag;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-t:#|wait time",&timeout,DATA_TYPE_INT,0,0,"10" },
		{ "-r|ignore persistant config",&read_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-n|do not ping agent",&no_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-l|list parameters",&list_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-f|list functions",&func_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-p|ping target(s)",&ping_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ ":target|agent name",&target,DATA_TYPE_STRING,sizeof(target)-1,1,0 },
		{ ":func|agent func",&func,DATA_TYPE_STRING,sizeof(func)-1,0,0 },
		OPTS_END
	};
	time_t start,now;
	config_function_t *f;
	solard_message_t *msg;
	int called,have_all;
#if TESTING
	char *args[] = { "sdconfig", "-d", "6", "-l", "Battery/jbd" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	c = client_init(argc,argv,opts,"sdconfig",0,0);
	if (!c) return 1;

	if (ping_flag) strcpy(func,"ping");

	dprintf(dlevel,"target: %s, func: %s, %d\n", target, func);
	if (!strlen(func) && !list_flag && !func_flag) {
		log_error("either list flag or func must be specified\n");
		return 1;
	}

        argc -= optind;
        argv += optind;
        optind = 0;

	sleep(1);
	if (!list_count(c->agents)) {
		sleep(1);
		if (!list_count(c->agents)) {
			log_error("no agents responded, aborting\n");
			return 1;
		}
	}

	if (list_flag || strcmp(func,"-l") == 0) {
		list_reset(c->agents);
		while((ap = list_get_next(c->agents)) != 0) {
			if (!client_matchagent(ap,target)) continue;
			do_list(ap);
		}
		return 0;
	}

	if (func_flag) {
		list_reset(c->agents);
		while((ap = list_get_next(c->agents)) != 0) {
			if (!client_matchagent(ap,target)) continue;
			if (!list_count(ap->funcs)) {
				printf("%s has no functions\n",target);
				return 0;
			}
			printf("%s functions:\n",ap->name);
			printf("  %-25.25s %s\n", "Name", "# args");
			list_reset(ap->funcs);
			while((f = list_get_next(ap->funcs)) != 0) printf("  %-25.25s %d\n", f->name, f->nargs);
		}
		return 0;
	}

	/* For each agent ... call the func */
	called = 0;
	list_reset(c->agents);
	while((ap = list_get_next(c->agents)) != 0) {
		if (!client_matchagent(ap,target)) continue;
		if ((f = client_getagentfunc(ap,func)) == 0) {
			printf("%s: function %s not found\n", ap->name, func);
			continue;
		}

		if (client_callagentfunc(ap,f,argc,argv)) {
			printf("%s: error calling function %s\n", ap->name, func);
			continue;
		}
		called++;
	}
	if (!called) return 0;

	/* get the replies... */
	time(&start);
	while(1) {
		have_all = 1;
		list_reset(c->mq);
		while((msg = list_get_next(c->mq)) != 0) {
			solard_message_dump(msg,6);
//			dprintf(dlevel,"mdg->id: %s, clientid: %s\n", msg->id, c->mqtt_config.clientid);
			if (strcmp(msg->id,c->mc.clientid) == 0) {
				ap = 0;
				if (strlen(msg->replyto)) ap = client_getagentbyid(c,msg->replyto);
				if (!ap) ap = client_getagentbyname(c,msg->id);
				if (ap) client_getagentstatus(ap,msg);
				dprintf(dlevel,"pub...\n");
				mqtt_pub(c->m, msg->topic, 0, 1, 0);
				sleep(1);
			}
			list_delete(c->mq,msg);
		}
		list_reset(c->agents);
		while((ap = list_get_next(c->agents)) != 0) {
			if (!client_matchagent(ap,target)) continue;
			dprintf(dlevel,"checking: %s...\n", ap->name);
			if (solard_check_bit(ap->state,CLIENT_AGENTINFO_STATUS)) {
				dprintf(dlevel,"got reply: status: %d, errmsg: %s\n", ap->status, ap->errmsg);
				printf("%s: %s\n", ap->name, ap->errmsg);
			} else {
				have_all = 0;
			}
		}
		if (have_all) break;
		time(&now);
		if ((now - start) > timeout)  {
			printf("timeout\n");
			break;
		}
		sleep(1);
	}
	return 0;
}
