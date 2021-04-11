
#include "common.h"
#include "smanet_internal.h"
#include "libyasdi.h"
#include "libyasdimaster.h"
#include <ctype.h>
#include <sys/types.h>
#include "master.h"

extern BOOL TSecurity_SetNewAccessLevel(char * user, char * passwd);

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f
#define CHANTYPE_ALL 0xffff

#define MAX_CHANHANDLES	2048

list smanet_getchanopt(smanet_session_t *s, char *chanName) {
	char valtext[128];
	int chan,r,count,i;
	list lp;

	chan = FindChannelName(s->dev_handle, chanName);
	dprintf(1,"chan: %d\n", chan);
	if (chan < 1) return 0;
	count = GetChannelStatTextCnt(chan);
	dprintf(1,"count: %d\n", count);
	lp = list_create();
	for(i=0; i < count; i++) {
		r = GetChannelStatText(chan, i, valtext, sizeof(valtext)-1);
		dprintf(1,"r: %d\n", r);
		if (r < 0) return 0;
		dprintf(1,"i: %d, text: %s\n", i, valtext);
		list_add(lp,valtext,strlen(valtext)+1);
	}
	return lp;
}

list smanet_getchanlist(smanet_session_t *s, int type) {
	DWORD ChanHandles[MAX_CHANHANDLES];
	char name[128];
	int r,i,count;
	list lp;

	dprintf(1,"handle: %d, type: %x\n", s->dev_handle, type);

	r = GetChannelHandles(s->dev_handle, ChanHandles, MAX_CHANHANDLES, type, 0);
	printf("r: %d\n", r);
	if (r < 0) return 0;
	count = r;
	lp = list_create();
	for(i=0; i < count; i++) {
		r = GetChannelName(ChanHandles[i], name, sizeof(name)-1);
		if (r < 0) return 0;
		dprintf(1,"adding: %s\n", name);
		list_add(lp,name,strlen(name)+1);
	}
	return lp;
}

int smanet_getparm(smanet_session_t *s, char *name, smanet_chaninfo_t *info) {
	int chan,r;
	char valtext[128];
	double cval;

	dprintf(1,"name: %s\n", name);

	chan = FindChannelName(s->dev_handle, name);
	dprintf(1,"chan: %d\n", chan);
	if (chan < 1) return 1;

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,s->dev_handle,&cval,valtext,sizeof(valtext)-1,86400);
	dprintf(1,"r: %d\n", r);
	if (r < 0) return 1;
	dprintf(1,"cval: %f, valtext: %s\n", cval, valtext);
	memset(info,0,sizeof(*info));
	strncat(info->name,name,sizeof(info->name)-1);
	dprintf(1,"%d: %s (%f)\n", chan, valtext, cval);
	if (strlen(valtext)) {
		info->type = DATA_TYPE_STRING;
		strncat(info->valtext,valtext,sizeof(info->valtext)-1);
	} else {
		info->type = DATA_TYPE_FLOAT;
		info->fval = cval;
	}
	return 0;
}

smanet_chaninfo_t *_smanet_find_chan(smanet_session_t *s,char *name) {
	smanet_chaninfo_t *cp;

	dprintf(1,"name: %s\n", name);

	list_reset(s->channels);
	while((cp = list_get_next(s->channels)) != 0) {
		if (strcmp(cp->name,name)==0) {
			dprintf(1,"found!\n");
			return cp;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

smanet_chaninfo_t *smanet_find_chan(smanet_session_t *s,char *name) {
	smanet_chaninfo_t *info;

	dprintf(1,"name: %s\n", name);
	info = _smanet_find_chan(s,name);
	dprintf(1,"info: %p\n", info);
	if (!info) {
		smanet_chaninfo_t newinfo;
#if 0
		int chan,r;
		float cval;
		char valtext[32];

		chan = FindChannelName(s->dev_handle, name);
		dprintf(1,"chan: %d\n", chan);
		if (chan < 0) return 0;
		r = GetChannelValue(chan,s->dev_handle,&cval,valtext,sizeof(valtext)-1,0);
		if (r < 0) return 0;
nt smanet_getparm(smanet_session_t *s, char *name, smanet_chaninfo_t *info) {
#endif
		if (smanet_getparm(s,name,&newinfo)) return 0;
		return _smanet_find_chan(s,name);

	}
	return info;
}

int smanet_setparm(smanet_session_t *s, char *name, char *sval) {
	int i,chan,count,r,found;
	char valtext[32];
	double cval,dval;

	dprintf(1,"name: %s, sval: %s\n", name, sval);

	chan = FindChannelName(s->dev_handle, name);
	dprintf(1,"chan: %d\n", chan);
	if (chan < 0) {
		printf("error: param not found\n");
		return 1;
	}

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,s->dev_handle,&cval,valtext,sizeof(valtext)-1,0);
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
	r = SetChannelValue(chan,s->dev_handle,dval);
	dprintf(1,"r: %d\n", r);
	if (r < 0) {
		printf("error: unable to set value: %f\n",dval);
		return 1;
	}
	return 0;
}

smanet_session_t *smanet_init(solard_module_t *tp, void *tp_handle) {
	smanet_session_t *s;
	DWORD devCount,deviceHandles[1];
	char device_type[32];
	int i,r;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"smanet_init: calloc");
		return 0;
	}
	s->tp = tp;
	s->tp_handle = tp_handle;
	s->channels = list_create();

	yasdiMasterInitialize(s);
	yasdiSetDriverOnline(0);
	r = yasdiDoMasterCmdEx("detect",1,0,0);
	if (r) return 0;
	GetDeviceHandles(deviceHandles, 1);
	dprintf(1,"deviceHandles[0]: %d\n", deviceHandles[0]);
	s->dev_handle = deviceHandles[0];

#if 0
	devCount = GetDeviceHandles(deviceHandles, 1);
	dprintf(1,"deviceHandles[0]: %d\n", deviceHandles[0]);
	for(i=0; i < devCount; i++) {
		GetDeviceType(deviceHandles[i], device_type, sizeof(device_type)-1);
		dprintf(1,"%d: device_type: %s\n", i, device_type);
	}
#endif

	TSecurity_SetNewAccessLevel(0,0);

#if 0
	{
		smanet_chaninfo_t info;
		smanet_getparm(s, "GdRmgTm", &info);
		exit(0);
	}
#endif

	return s;
}
#if 0
  TMasterCmdResult CmdState;
   TMasterCmdReq * mc;
TMasterCmdType MasterCmd;
      mc = TMasterCmdFactory_GetMasterCmd( MC_DETECTION );
      mc->wDetectDevMax = 1;
      mc->iDetectMaxTrys = 1;
      TSMADataMaster_AddCmd( mc );
      TMasterCmd_WaitFor( mc );
      TMasterCmdFactory_FreeMasterCmd( mc );
	return s;

      //Request (master) command to get new values...
	MasterCmd = MC_DETECTION;
//	MasterCmd = MC_RESET;
//	MasterCmd = MC_GET_SPOTCHANNELS;
      mc = TMasterCmdFactory_GetMasterCmd( MasterCmd );
      mc->Param.DevHandle  = 0;
      mc->Param.ChanHandle = 0xFFFF;
      mc->Param.dwValueAge = 99;
      TSMADataMaster_AddCmd( mc );
      CmdState = TMasterCmd_WaitFor( mc ); //wait for completion...
      TMasterCmdFactory_FreeMasterCmd( mc );
	return s;
	yasdiDoMasterCmdEx("detect",1,0,0);
#endif
