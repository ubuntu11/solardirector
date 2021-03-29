
#include "common.h"
#include "smanet_internal.h"
#include "libyasdi.h"
#include "libyasdimaster.h"
#include <ctype.h>
#include <sys/types.h>

extern BOOL TSecurity_SetNewAccessLevel(char * user, char * passwd);

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f
#define CHANTYPE_ALL 0xffff

#define MAX_CHANHANDLES	2048


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

smanet_session_t *smanet_init(solard_module_t *tp, void *tp_handle) {
	smanet_session_t *s;
	DWORD drivers[1],devCount,deviceHandles[1];
	char name[32],device_type[32];
	int i,driverCount,found,r,done;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"smanet_init: calloc");
		return 0;
	}
	s->tp = tp;
	s->tp_handle = tp_handle;

	yasdiMasterInitialize(s);

#if 0
	driverCount = yasdiMasterGetDriver(drivers, 1);
	dprintf(1,"driver[0]: %d\n", drivers[0]);
	dprintf(1,"driverCount: %d\n",driverCount);
	if (driverCount < 0 || driverCount > 5) {
		printf("error initializing drivers\n");
		return 0;
	}

	found=0;
	for(i=0; i < driverCount; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		dprintf(1,"name: %s\n", name);
		dprintf(1,"drivers[0]: %d\n", drivers[0]);
		if (yasdiSetDriverOnline(drivers[i])) {
			dprintf(1,"Driver: %s: Online\n", name);
			found=1;
		} else {
			printf("Driver: %s: FAILED\n",name);
		}
	}
	if (!found) {
		printf("error: no drivers online\n");
		return 0;
	}
#endif

	yasdiSetDriverOnline(0);

#if 1
	r = yasdiDoMasterCmdEx("detect",1,0,0);
	dprintf(1,"r: %d\n", r);
#else
	done = 0;
	while(!done) {
		r = DoStartDeviceDetection(1, TRUE);
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
			return 0;
			break;
		default:
			printf("error: %d\n", r);
			return 0;
			break;
		}
	}
#endif

	devCount = GetDeviceHandles(deviceHandles, 1);
	dprintf(1,"deviceHandles[0]: %d\n", deviceHandles[0]);
	for(i=0; i < devCount; i++) {
		GetDeviceType(deviceHandles[i], device_type, sizeof(device_type)-1);
		dprintf(1,"%d: device_type: %s\n", i, device_type);
	}

	TSecurity_SetNewAccessLevel(0,0);

//	r = dolist(deviceHandles[0], CHANTYPE_PARM, "RnMod");
	r = getparm(1, "GdPwrEna");

	return s;
}
#if 0
	int opt,all,list,param,action,type;
	char *progName,*configFile,name[32],device_type[32];
	cfg_info_t *cfg;
	char *pname, *pval;

	progName = "yes";

	list = 0;
	if (list) {
		action = 0;
		if (all) {
			type = CHANTYPE_ALL;
		} else if (param) {
			type = CHANTYPE_PARM;
		} else {
			type = CHANTYPE_SPOT;
		}
	} else if (1 == 2) {
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
	count = 1;


	dprintf(1,"action: %d\n", action);
	switch(action) {
	case 0:
		r = dolist(deviceHandles[0], type, (pname ? pname : 0));
		break;
	case 1:
		r = getparm(deviceHandles[0], pname);
		break;
	case 2:
		r = setparm(deviceHandles[0], pname, pval);
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
#endif
