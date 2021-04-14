
#include "libyasdi.h"
#include "libyasdimaster.h"
#include "debug.h"

#include "netdevice.h"
#include "netchannel.h"

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
//0xffff = all
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f
#define CHANTYPE_ALL 0xffff

#define MAX_CHANHANDLES	4096

int main(void) {
	int i,j,r,count,driverCount,found,done;
	DWORD ChanHandles[MAX_CHANHANDLES];
	char name[32],type[32];
	DWORD drivers[5];
#if 0
	int count,i,j,driverCount,found,r,done,chan,retries;
	DWORD devCount;
	DWORD deviceHandles[1];
	double val;
	char valtext[32];
	DWORD ChanHandles[MAX_CHANHANDLES];
	TNewChanListFilter filter;
	int r;

   DWORD i = 0;
   TChannel * CurChan;
   TNetDevice * dev;
   int ii,res;
#endif

	yasdiReset();
	yasdiMasterInitialize("yasdi.ini",&count);

	driverCount = yasdiMasterGetDriver(drivers, 5);
	printf("count: %d, driverCount: %d\n",count,driverCount);

	found=0;
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		if (yasdiSetDriverOnline(drivers[i])) {
			printf("Driver: %s: Online\n", name);
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
		r = DoStartDeviceDetection(1, TRUE);
		printf("r: %d\n", r);
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
	}
	dprintf("deviceHandles[0]: %d\n",(int)deviceHandles[0]);

#if 0
	devCount = GetDeviceHandles(deviceHandles, 1);
	for(i=0; i < devCount; i++) {
		GetDeviceType(deviceHandles[i], type, sizeof(type)-1);
		printf("Device type: %s\n", type);
#if 0
		r = GetChannelHandles(deviceHandles[i], ChanHandles, MAX_CHANHANDLES, CHANTYPE_PARM, 0);
		printf("r: %d\n", r);
		for(j=0; j < r; j++) {
			GetChannelName( ChanHandles[j], name, sizeof(name)-1);
			printf("%d: %s\n", j, name);
		}
#endif
	}
#endif
	r = GetChannelHandles(0, ChanHandles, MAX_CHANHANDLES, 0xffff, 0);
	printf("r: %d\n", r);
	for(j=0; j < r; j++) {
		GetChannelName( ChanHandles[j], name, sizeof(name)-1);
		printf("%d: %s\n", j, name);
	}
#if 0
	chan = FindChannelName(deviceHandles[0], "GdManStr");
//	chan = FindChannelName(deviceHandles[0], "ExtPwrAt");
	printf("chan: %d\n", r);
	if (chan < 0) goto error;
	GetChannelName(chan, name, sizeof(name)-1);
	printf("%d: %s\n", chan, name);
	val = 0.0;
	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,deviceHandles[0],&val,valtext,sizeof(valtext)-1,99);
	printf("r: %d\n", r);
	if (r < 0) goto error;
	printf("val: %lf\n",val);
	printf("valtext: %s\n", valtext);
//	if (val < 3.0) val = 3.0;
//	else val = 2.0;
	val = 1.0;
	r = SetChannelValue(chan,deviceHandles[0],val);
	printf("r: %d\n", r);
	if (r < 0) goto error;
	r = GetChannelValue(chan,deviceHandles[0],&val,valtext,sizeof(valtext)-1,99);
	printf("r: %d\n", r);
	if (!r) {
		printf("val: %lf\n",val);
		printf("valtext: %s\n", valtext);
	}
#endif

error:
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		yasdiSetDriverOffline(drivers[i]);
		printf("Driver: %s: Offline\n", name);
	}
	yasdiMasterShutdown();
	return 0;
}
