
#include "common.h"
#include <ctype.h>
#include <sys/types.h>
#include "smanet_internal.h"

int debug = 5;

#if 0
#include "os.h"
#include "states.h"

#include "master.h"
#include "debug.h"
#include "smadata_layer.h"
#include "netdevice.h"
#include "plant.h"
#include "mastercmd.h"

#include "libyasdi.h"
#include "libyasdimaster.h"

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f
#define CHANTYPE_ALL 0xffff

#define MAX_CHANHANDLES	2048

extern BOOL TSecurity_SetNewAccessLevel(char * user, char * passwd);

int dolist(int handle, int type, char *chanName) {
	DWORD ChanHandles[MAX_CHANHANDLES];
	char valtext[32],name[32];
	int r,i,count;

	dprintf(1,"handle: %d, type: %x, chanName: %s\n", handle, type, chanName);

	/* If name specified, list possible options for chan */
	if (chanName) {
		int chan;
		char valtext[32];

		chan = FindChannelName(handle, chanName);
		dprintf(1,"chan: %d\n", chan);
		if (chan < 1) {
			printf("error: channel name not found\n");
			return 1;
		}
		count = GetChannelStatTextCnt(chan);
		dprintf(1,"count: %d\n", count);
		if (!count) {
			printf("channel has no option names\n");
			return 0;
		}
		for(i=0; i < count; i++) {
			r = GetChannelStatText(chan, i, valtext, sizeof(valtext)-1);
			dprintf(1,"r: %d\n", r);
			if (r < 0) {
				printf("error: unable to get channel option\n");
				return 1;
			}
			dprintf(1,"i: %d, text: %s\n", i, valtext);
			printf("%s\n",valtext);
		}
	} else {
		memset(valtext,0,sizeof(valtext));
		r = GetChannelHandles(handle, ChanHandles, MAX_CHANHANDLES, type, 0);
		printf("r: %d\n", r);
		if (r < 0) return 1;
		count = r;
		for(i=0; i < count; i++) {
			r = GetChannelName(ChanHandles[i], name, sizeof(name)-1);
			if (r < 0) {
				printf("list: error getting channel name for channel %d!\n",i);
				return 1;
			}
			printf("%d: %s\n", i, name);
		}
	}
	return 0;
}

int getparm(int handle, char *name) {
	int chan,r;
	char valtext[32];
	double cval;

	dprintf(1,"name: %s\n", name);

	chan = FindChannelName(handle, name);
	dprintf(1,"chan: %d\n", chan);
	if (chan < 1) {
		printf("error: channel name not found\n");
		return 1;
	}

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,handle,&cval,valtext,sizeof(valtext)-1,99);
	dprintf(1,"r: %d\n", r);
	if (r < 0) {
		printf("error: unable to get current value\n");
		return 1;
	}
	dprintf(1,"cval: %f, valtext: %s\n", cval, valtext);

	dprintf(1,"%d: %s (%f)\n", chan, valtext, cval);
	if (strlen(valtext))
		printf("%s\n",valtext);
	else
		printf("%f\n",cval);
	return 0;
}

int setparm(int handle, char *name, char *sval) {
	int i,chan,count,r,found;
	char valtext[32];
	double cval,dval;

	dprintf(1,"name: %s, sval: %s\n", name, sval);

	chan = FindChannelName(handle, name);
	dprintf(1,"chan: %d\n", chan);
	if (chan < 0) {
		printf("error: param not found\n");
		return 1;
	}

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,handle,&cval,valtext,sizeof(valtext)-1,99);
	dprintf(1,"r: %d\n", r);
	if (r < 0) {
		printf("error: unable to get current value\n");
		return 1;
	}
	dprintf(1,"cval: %f, valtext: %s\n", cval, valtext);

	count = GetChannelStatTextCnt(chan);
	dprintf(1,"count: %d\n", count);
	if (count) {
		dprintf(1,"current value: %s\n", valtext);
		if (strcmp(sval,valtext)==0) return 0;
		found = 0;
		for(i=0; i < count; i++) {
			r = GetChannelStatText(chan, i, valtext, sizeof(valtext)-1);
			dprintf(1,"r: %d\n", r);
			if (r < 0) {
				printf("error: unable to get channel option\n");
				return 1;
			}
			dprintf(1,"i: %d, text: %s\n", i, valtext);
			if (strcmp(valtext,sval)==0) {
				dval = i;
				found = 1;
				break;
			}
		}
		dprintf(1,"found: %d\n", found);
		if (!found) {
			printf("error: option not found\n");
			return 1;
		}
	} else {
		dval = atof(sval);
		dprintf(1,"cval: %f, dval: %f\n", cval, dval);
		if (dval == cval) return 0;
	}
	r = SetChannelValue(chan,handle,dval);
	dprintf(1,"r: %d\n", r);
	if (r < 0) {
		printf("error: unable to set value: %f\n",dval);
		return 1;
	}
	return 0;
}

static int getmyhomedir(char *dest, int destlen) {
	FILE *fp;
	char line[256],*p,*s;
	long uid;
	int len,id,i;
//	struct passwd *pw;

	uid = getuid();
	fp = fopen("/etc/passwd","r");
	if (!fp) {
		perror("fopen /etc/passwd");
		return 1;
	}
	while(fgets(line,sizeof(line)-1,fp)) {
		len = strlen(line);
		if (!len) continue;
		while(len > 1 && isspace(line[len-1])) len--;
		dprintf(1,"line[%d]: %d\n", len, line[len]);
		line[len] = 0;
		dprintf(1,"line: %s\n", line);
		p = line;
		for(i=0; i < 2; i++) {
			p = strchr(p+1,':');
			if (!p) goto getmyhomedir_error;
			dprintf(1,"p(%d): %s\n", i, p);
		}
		s = p+1;
		p = strchr(s,':');
		if (p) *p = 0;
		dprintf(1,"s: %s\n", s);
		id = atoi(s);
		if (id == uid) {
			for(i=0; i < 2; i++) {
				p = strchr(p+1,':');
				if (!p) goto getmyhomedir_error;
				dprintf(1,"p(%d): %s\n", i, p);
			}
			s = p+1;
			p = strchr(s,':');
			if (p) *p = 0;
			dprintf(1,"s: %s\n", s);
			dest[0] = 0;
			strncat(dest,s,destlen);
			len = strlen(dest);
			if (dest[len] != '/') strcat(dest,"/");
			fclose(fp);
			return 0;
		}
	}
getmyhomedir_error:
	fclose(fp);
	return 1;
}

char *find_config_file(char *name) {
	static char temp[1024];

	if (access(name,R_OK)==0) return name;
	if (!getmyhomedir(temp,sizeof(temp)-1)) {
		strcat(temp,"etc/");
		strcat(temp,name);
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

void usage(char *myname) {
	printf("usage: %s [-alps] -n chan | ChanName [value]\n",myname);
	return;
}

int main(int argc, char **argv) {
	int opt,all,list,param,action,type;
	int i,driverCount,found,r,done;
	char *progName,*configFile,name[32],device_type[32];
	DWORD drivers[5],devCount,count,deviceHandles[1];
	char transport[32],target[64],topts[32];
	cfg_info_t *cfg;
	cfg_proctab_t tab[] = {
		{ "siutil","transport","Device transport",DATA_TYPE_STRING,transport,SOLARD_TRANSPORT_LEN-1, "" },
		{ "siutil","target","Device target",DATA_TYPE_STRING,target,SOLARD_TARGET_LEN-1, "" },
		{ "siutil","topts","Device transport options",DATA_TYPE_STRING,topts,SOLARD_TOPTS_LEN-1, "" },
		CFG_PROCTAB_END
	};
	smanet_session_t *s;

	progName = argv[0];

	all = list = param = 0;
	while ((opt=getopt(argc, argv, "alph")) != -1) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
		case 'l':
			list = 1;
			break;
		case 'p':
			param = 1;
			break;
		default:
		case 'h':
			usage(progName);
			exit(0);
			break;
		}
	}
	dprintf(1,"all: %d, list: %d, param: %d\n", all, list, param);

	if (all && !list) printf("info: all flag (-a) only applies to list (-l), ignored.\n");

	argc -= optind;
	argv += optind;

	for(i=0; i < argc; i++) dprintf(1,"arg[%d]: %s\n",i,argv[i]);

	dprintf(1,"argc: %d\n", argc);
	if (argc < 1 && list == 0) {
		usage(progName);
		return 1;
	}
	if (list) {
		action = 0;
		if (all) {
			type = CHANTYPE_ALL;
		} else if (param) {
			type = CHANTYPE_PARM;
		} else {
			type = CHANTYPE_SPOT;
		}
	} else if (argc < 2) {
		action = 1;
	} else {
		action = 2;
	}

	configFile = find_config_file("siutil.conf");
	if (!configFile) {
		printf("error: unable to find siutil.conf\n");
		return 1;
	}
	dprintf(1,"configFile: %s\n", configFile);
	cfg = cfg_read(configFile);
	if (!cfg) {
		perror("cfg_read");
		return 1;
	}
	dprintf(1,"cfg: %p\n", cfg);
	cfg_get_tab(cfg,tab);
	dprintf(1,"got tab\n");
	if (debug) cfg_disp_tab(tab,"siutil",0);

	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	if (!strlen(transport) || !strlen(target)) {
		log_write(LOG_ERROR,"must provide both transport and target\n");
		return 1;
	}

	s = calloc(1,sizeof(*s));
	if (!s) return 1;
	s->tp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
	s->tp_handle = s->tp->new(0,target,topts);
//	s = smanet_init(tp, tp_handle);

	yasdiMasterInitialize(s);
	count = 1;

	driverCount = yasdiMasterGetDriver(drivers, 5);
	dprintf(1,"driverCount: %d\n",driverCount);
	if (driverCount < 0 || driverCount > 5) {
		printf("error initializing drivers\n");
		return 1;
	}

	found=0;
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		if (yasdiSetDriverOnline(drivers[i])) {
			dprintf(1,"Driver: %s: Online\n", name);
			found=1;
		} else {
			printf("Driver: %s: FAILED\n",name);
		}
	}
	if (!found) {
		printf("error: no drivers online\n");
		return 1;
	}

	done = 0;
	while(!done) {
		r = DoStartDeviceDetection(1, FALSE);
		dprintf(1,"r: %d\n", r);
		switch(r) {
		case YE_OK:
			done = 1;
			break;
		case YE_NOT_ALL_DEVS_FOUND:
			sleep(1);
			break;
		case YE_DEV_DETECT_IN_PROGRESS:
			printf("error: another detection is in progress\n");
			goto error;
			break;
		default:
			printf("error: %d\n", r);
			goto error;
			break;
		}
		sleep(1);
	}

	TSecurity_SetNewAccessLevel(0,0);

	devCount = GetDeviceHandles(deviceHandles, 1);
	dprintf(1,"deviceHandles[0]: %d\n", deviceHandles[0]);
	for(i=0; i < devCount; i++) {
		GetDeviceType(deviceHandles[i], device_type, sizeof(device_type)-1);
		dprintf(1,"%d: device_type: %s\n", i, device_type);
	}

	dprintf(1,"action: %d\n", action);
	switch(action) {
	case 0:
		r = dolist(deviceHandles[0], type, (argc ? argv[0] : 0));
		break;
	case 1:
		r = getparm(deviceHandles[0], argv[0]);
		break;
	case 2:
		r = setparm(deviceHandles[0], argv[0], argv[1]);
		break;
	}

error:
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		yasdiSetDriverOffline(drivers[i]);
		dprintf(1,"Driver: %s: Offline\n", name);
	}
	yasdiMasterShutdown();
	return r;
}
#else
int main(int argc, char **argv) {
	smanet_session_t *s;
	solard_module_t *tp;
	void *tp_handle;
	char *transport;

	transport = "serial";
	tp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
	tp_handle = tp->new(0,"/dev/ttyS0","19200");
	s = smanet_init(tp, tp_handle);
	if (!s) return 1;
}
#endif
