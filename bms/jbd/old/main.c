/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parson.h"
#endif
#include "agent.h"

#if 0
int debug = 9;

JSON_Value *root_value;
JSON_Object *root_object;
char *serialized_string = NULL;

#define dint(l,f,v) json_object_set_number(root_object, l, v)

#define dblool(l,f,v) json_object_set_boolean(root_object, l, v)

#define dfloat(l,f,v) json_object_set_number(root_object, l, v)

#define dstr(l,f,v) json_object_set_string(root_object, l, v)

void display_info(jbd_info_t *info) {
	char temp[256],*p;
	int i;

	dfloat("Voltage","%.3f",info->voltage);
	dfloat("Current","%.3f",info->current);
	dfloat("DesignCapacity","%.3f",info->fullcap);
	dfloat("RemainingCapacity","%.3f",info->capacity);
	dint("PercentCapacity","%d",info->pctcap);
	dint("CycleCount","%d",info->cycles);
	dint("Probes","%d",info->probes);
	p = temp;
	p += sprintf(p,"[ ");
	for(i=0; i < info->probes; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p, "%.1f",info->temps[i]);
	}
	strcat(temp," ]");
	dprintf(1,"temp: %s\n", temp);
	json_object_dotset_value(root_object, "Temps", json_parse_string(temp));
	dint("Strings","%d",info->strings);
	p = temp;
	p += sprintf(p,"[ ");
	for(i=0; i < info->strings; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p, "%.3f",info->cellvolt[i]);
	}
	strcat(temp," ]");
	dprintf(1,"temp: %s\n", temp);
	json_object_dotset_value(root_object, "Cells", json_parse_string(temp));
	dfloat("CellTotal","%.3f",info->cell_total);
	dfloat("CellMin","%.3f",info->cell_min);
	dfloat("CellMax","%.3f",info->cell_max);
	dfloat("CellDiff","%.3f",info->cell_diff);
	dfloat("CellAvg","%.3f",info->cell_avg);
	temp[0] = 0;
	p = temp;
	if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,"Charge");
	if (info->fetstate & JBD_MOS_DISCHARGE) {
		if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,",");
		p += sprintf(p,"Discharge");
	}
	dstr("FET","%s",temp);
#if 0
        unsigned long balancebits;
        /* the protection sign */
        unsigned short protectbits;
        struct {
                unsigned sover: 1;              /* Single overvoltage protection */
                unsigned sunder: 1;             /* Single undervoltage protection */
                unsigned gover: 1;              /* Whole group overvoltage protection */
                unsigned gunder: 1;             /* Whole group undervoltage protection */
                unsigned chitemp: 1;            /* Charge over temperature protection */
                unsigned clowtemp: 1;           /* Charge low temperature protection */
                unsigned dhitemp: 1;            /* Discharge over temperature protection */
                unsigned dlowtemp: 1;           /* Discharge low temperature protection */
                unsigned cover: 1;              /* Charge overcurrent protection */
                unsigned cunder: 1;             /* Discharge overcurrent protection */
                unsigned shorted: 1;            /* Short circuit protection */
                unsigned ic: 1;                 /* Front detection IC error */
                unsigned mos: 1;                /* Software lock MOS */
        } protect;
        unsigned short fetstat;                 /* for the MOS tube status */
        struct {
                unsigned charging: 1;
                unsigned discharging: 1;
        } fet;
#endif
}

int init_pack(mybmm_pack_t *pp, mybmm_config_t *c, char *type, char *transport, char *target, char *opts, mybmm_module_t *cp, mybmm_module_t *tp) {
	dprintf(1,"pp: %p, c: %p, type: %s, transport: %s, target: %s, opts: %s, tp: %p\n",
		pp, c, type ? type : "", transport ? transport : "", target ? target : "", opts ? opts : "", tp);
	memset(pp,0,sizeof(*pp));
	strcpy(pp->type,type);
	if (transport) strcpy(pp->transport,transport);
	if (target) strcpy(pp->target,target);
	if (opts) strcpy(pp->opts,opts);
        pp->open = cp->open;
        pp->read = cp->read;
        pp->close = cp->close;
        pp->handle = cp->new(c,pp,tp);
	list_add(c->packs,pp,sizeof(pp));
        return 0;
}

enum JBDTOOL_ACTION {
	JBDTOOL_ACTION_INFO=0,
	JBDTOOL_ACTION_READ,
	JBDTOOL_ACTION_WRITE,
	JBDTOOL_ACTION_LIST
};

static int pid;
static int term = 0;
void catch_alarm(int sig) {
	printf("timeout expired, killing pid %d\n", (int)pid);
        if (term)
                kill(pid,SIGKILL);
        else {
                kill(pid,SIGTERM);
                term++;
                alarm(3);
        }
}

int pretty;
int get_info(jbd_session_t *s,mqtt_session_t *m) {
	jbd_info_t info;
	int r;

	dprintf(1,"s: %p\n",s);
	if (s->pp->open(s)) return 1;
	r = jbd_get_info(s,&info);
	dprintf(1,"r: %d\n", r);
	if (!r) {
		display_info(&info);
		if (pretty)
    			serialized_string = json_serialize_to_string_pretty(root_value);
		else
    			serialized_string = json_serialize_to_string(root_value);
		mqtt_send(m, serialized_string, 15);
		json_free_serialized_string(serialized_string);
	}
	s->pp->close(s);
	return r;
}

int bgrun(jbd_session_t *s, mqtt_session_t *m, int timeout) {
//	int pty;

	dprintf(1,"s: %p, timeout: %d\n", s, timeout);
#if 0
	if ((pty = getpt()) < 0) {
		perror("openpt");
		return -1;
	}
	if (grantpt(pty)) {
		perror("granpt");
		return -1;
	}
	if (unlockpt(pty)) {
		perror("unlockpt");
		return -1;
	}
#endif

	dprintf(1,"forking...\n");
	pid = fork();
	if (pid < 0) {
		/* Error */
		perror("fork");
		return -1;
	} else if (pid == 0) {
#if 0
		/* Child */
		int pts;
		struct termios old_tcattr,tcattr;

		/* Detach from current tty */
		setsid();

		/* Open pty slave */
		pts = open(ptsname(pty), O_RDWR );
		if (pts < 0) perror("open pts");

		/* Close master */
		close(pty);

		/* Set raw mode term attribs */
		tcgetattr(pts, &old_tcattr); 
		tcattr = old_tcattr;
		cfmakeraw (&tcattr);
		tcsetattr (pts, TCSANOW, &tcattr); 

		/* Set stdin/stdout/stderr with pts */
		dup2(pts, STDIN_FILENO);
		dup2(pts, STDOUT_FILENO);
		dup2(pts, STDERR_FILENO);
#endif

		/* Call func */
		dprintf(1,"updating...\n");
		_exit(get_info(s,m));
	} else {
		/* Parent */
		struct sigaction act1, oact1;
		int status;

		/* Set up timeout alarm */
		if (timeout) {
			act1.sa_handler = catch_alarm;
			sigemptyset(&act1.sa_mask);
			act1.sa_flags = 0;
#ifdef SA_INTERRUPT
			act1.sa_flags |= SA_INTERRUPT;
#endif
			if( sigaction(SIGALRM, &act1, &oact1) < 0 ){
				perror("sigaction");
				exit(1);
			}
			alarm(timeout);
		}

		dprintf(1,"waiting on pid...\n");
		waitpid(pid,&status,0);
		dprintf(1,"WIFEXITED: %d\n", WIFEXITED(status));
		if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		dprintf(1,"status: %d\n", status);
		return status;
	}
	return 0;
}

void usage() {
	printf("usage: jbdtool [-abcjJrwlh] [-f filename] [-t <module:target> [-o output file]\n");
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c <filename>	specify config file\n");
	printf("  -J		preety-print JSON output\n");
	printf("  -l <logfile>	write output to logfile\n");
	printf("  -h		this output\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -m 		Send results to MQTT broker\n");
	printf("  -i 		Update interval\n");
	printf("  -b 		Run in background\n");
}

int become_daemon(void);

int main(int argc, char **argv) {
	char *transport,*target,*logfile,*configfile,*name;
	mybmm_config_t *conf;
	mybmm_module_t *cp,*tp;
	mybmm_pack_t pack;
	int opt,back,interval;
	char *mqtt;
	time_t start,end,diff;
	char clientid[32],topic[192];
	mqtt_session_t *m;

	log_open("mybmm",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	transport = target = logfile = configfile = name = 0;
	interval = back = pretty = 0;
	mqtt = 0;
	clientid[0] = 0;
	while ((opt=getopt(argc, argv, "+c:d:bm:i:t:l:hJn:")) != -1) {
		switch (opt) {
		case 'c':
			configfile = optarg;
			break;
		case 'd':
			debug=atoi(optarg);
			break;
		case 'b':
			back = 1;
			break;
		case 'm':
			mqtt = optarg;
			break;
		case 'n':
			name = optarg;
			break;
		case 'J':
			pretty = 1;
			break;
                case 'i':
			interval=atoi(optarg);
			break;
                case 't':
			transport = optarg;
			target = strchr(transport,':');
			if (!target) {
				printf("error: format is transport:target\n");
				usage();
				return 1;
			}
			*target = 0;
			target++;
			break;
                case 'l':
			logfile = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(0);
                }
        }
	if (!name) name = "pack";
	if (logfile) log_open("mybmm",logfile,LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	dprintf(1,"configfile: %p\n", configfile);
	if (configfile) {
		conf->cfg = cfg_read(configfile);
		dprintf(1,"cfg: %p\n", conf->cfg);
		if (!conf->cfg) {
			lprintf(LOG_SYSERR,"cfg_read");
			return 1;
		}
		memset(&pack,0,sizeof(pack));
		dprintf(1,"name: %s\n",name);
		pack_add(conf,name,&pack);
		mqtt_init(conf);
	} else {
		if (!transport) {
			printf("error: transport must be specified if no configfile\n");
			return 1;
		}
		tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
		if (!tp) {
			lprintf(LOG_ERROR,"unable to load transport module: %s\n",transport);
			return 1;
		}
		cp = mybmm_load_module(conf,"jbd",MYBMM_MODTYPE_CELLMON);
		if (!cp) {
			lprintf(LOG_ERROR,"unable to load cellmon module: %s\n","jbd");
			return 1;
		}

		/* Init the pack */
		if (init_pack(&pack,conf,"jbd",transport,target,0,cp,tp)) return 1;
	}
	if (!list_count(conf->packs)) {
		printf("error: no pack defined!  define one on the command line or in a config file!\n");
		return 1;
	}

	/* If MQTT, output is compact JSON */
	dprintf(1,"mqtt: %p\n", mqtt);
	if (mqtt) {
		strcpy(conf->mqtt_broker,strele(0,",",mqtt));
		strcpy(clientid,strele(1,",",mqtt));
		strcpy(conf->mqtt_topic,strele(2,",",mqtt));
		dprintf(1,"broker: %s, clientid: %s, topic: %s\n", conf->mqtt_broker, clientid, conf->mqtt_topic);
	}
	if (!strlen(conf->mqtt_broker)) strcpy(conf->mqtt_broker,"127.0.0.1");
	if (!strlen(conf->mqtt_topic)) {
		printf("error: MUST provide MQTT topic info!\n");
		return 1;
	}
	if (!strlen(clientid)) strcpy(clientid,name);

	/* Create full topic name from topic + /clientid  */
	sprintf(topic,"%s/%s",conf->mqtt_topic,clientid);
	dprintf(1,"topic: %s\n", topic);

	/* Create a new MQTT session and connect to the broker */
	m = mqtt_new(conf->mqtt_broker,clientid,topic);

	lprintf(LOG_INFO,"Connecting to Broker...\n");
	if (mqtt_connect(m,interval ? interval/2 : 20)) return 1;

	dprintf(1,"back: %d\n", back);
	if (back) {
		become_daemon();
		if (logfile) log_open("mybmm",logfile,LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);
	}

	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	do {
		time(&start);
		bgrun(pack.handle,m,interval);
		time(&end);
		diff = end - start;
		dprintf(1,"start: %d, end: %d, diff: %d\n", (int)start, (int)end, (int)diff);
		if (diff < interval) sleep(interval-(int)diff);
	} while(interval);
	json_value_free(root_value);
	mqtt_disconnect(m,5);
	mqtt_destroy(m);
	return 0;
}
#endif

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
	char temp[128];

	ap = agent_init(argc,argv,"jbd",SOLARD_MODTYPE_BATTERY,opts);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
//	dprintf(1,"ap->topic: %s\n", ap->topic);
//	sprintf(temp,"%s/JBD/Config/+",ap->topic);
//	dprintf(1,"temp: %s\n",temp);
//	mqtt_sub(ap->mqtt_handle,temp);
	agent_run(ap);
	return 0;
}
