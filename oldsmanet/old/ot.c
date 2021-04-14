
#include "libyasdi.h"
//#include "libyasdimaster.h"
#include "debug.h"

#include "netdevice.h"
#include "netchannel.h"

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f

#define MAX_CHANHANDLES	4096

static int libResolveHandles(DWORD devHandle, DWORD chanHandle, TNetDevice ** dev, TChannel ** chan, const char * funcName)
{
        dprintf("devHandle: %d, chanHandle: %d, dev: %p, chan: %p, funcName: %p\n",
                devHandle, chanHandle, dev, chan, funcName);

        //resolve device handle...
        if (dev && devHandle != INVALID_HANDLE)
        {
                *dev = TObjManager_GetRef( devHandle  );
                if (*dev == NULL)
                {
			dprintf("unknown handle");
			return 1;
                }
        }

        //resolve channel handle...
        if (chan && chanHandle != INVALID_HANDLE)
        {
                *chan = TObjManager_GetRef( chanHandle );
                if (*chan == NULL)
                {
                        YASDI_DEBUG((VERBOSE_LIBRARY,
                      "ERROR: %s(): Called with unknown channel handle"
                      " (%u)!\n", funcName, chanHandle));
			return 1;
                }
        }

	return 0;
}

#if 0
int GetChannelName(int chan, char *name, int namelen) {
	char * cNamePtr = TChannel_GetName( chan );
      if (cNamePtr)
      {
                   int iSize = min( ChanNameMaxBuf, strlen(cNamePtr) );
         strncpy(ChanName, cNamePtr, iSize);

         //Bugfix: YSD-15: Trim zero termination into destionation buffer
         iSize = min((int)ChanNameMaxBuf-1, iSize );

         //add string NULL termination
                   ChanName[ iSize ] = 0;
      }
#endif

int main(void) {
#if 0
	int count,i,j,driverCount,found,r,done,chan,retries;
	DWORD drivers[5];
	DWORD devCount;
	DWORD deviceHandles[1];
	double val;
	char valtext[32];
#endif
	char name[32],type[32];
	DWORD ChanHandles[MAX_CHANHANDLES];
	TNewChanListFilter filter;
	int r;

   DWORD i = 0;
   TChannel * CurChan;
   TNetDevice * dev;
   int ii,res;

   res = libResolveHandles(0, INVALID_HANDLE, &dev, NULL, __func__ );
        dprintf("res: %d\n", res);
   if (res != 0 ) return res;


   //iter all channels and copy handle...
   TNewChanListFilter_Init(&filter, 0xffff, 0, LEV_3);
//#define GET_NEXT_CHANNEL(i, list, filter) (TChannel*)TNewChanListFilter_GetNextElement( list, &i, filter)
//#define FOREACH_CHANNEL(i, list, chan, filter) for(i = 0; (chan = GET_NEXT_CHANNEL(i, list, filter) ) != NULL; )
   FOREACH_CHANNEL(i, TNetDevice_GetChannelList(dev), CurChan, &filter)
   {
      if (i < MAX_CHANHANDLES) {
		ChanHandles[i++] = CurChan->Handle;
		printf("%d: %s\n", i, CurChan->Name);
	}
	else
		break;
   }
#if 0
struct _TNewChanListFilter;
void * TNewChanListFilter_GetNextElement(THandleList * list, int * iter, struct _TNewChanListFilter * filter);
#endif


#if 0
	yasdiMasterInitialize("yasdi.ini",&count);

	driverCount = yasdiMasterGetDriver(drivers, 5);
	printf("count: %d, driverCount: %d\n",driverCount);

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
exit(0);

	devCount = GetDeviceHandles(deviceHandles, 1);
	for(i=0; i < devCount; i++) {
#if 0
		GetDeviceName(deviceHandles[i], name, sizeof(name)-1);
		printf("Device name: %s\n", name);
#endif
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
#if 0
	r = GetChannelHandles(0, ChanHandles, MAX_CHANHANDLES, 0xffff, 0);
	printf("r: %d\n", r);
	for(j=0; j < r; j++) {
		GetChannelName( ChanHandles[j], name, sizeof(name)-1);
		printf("%d: %s\n", j, name);
	}
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

error:
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		yasdiSetDriverOffline(drivers[i]);
		printf("Driver: %s: Offline\n", name);
	}
	yasdiMasterShutdown();
#endif
	return 0;
}
