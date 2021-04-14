
#include "os.h"
#include "debug.h"
#include "smadef.h"
#include "repository.h"
#include "device.h"
#include "driver_layer.h"
#include "smanet_internal.h"

/**************************************************************************
***** LOCAL - Prototyps ***********************************************
***************************************************************************/

void transport_close(int fd);
BOOL transport_reopen(TDevice * dev); 
//void transport_modemstatus_set(TDevice * dev, DWORD flags);
//DWORD transport_modem_get_status(TDevice * dev, DWORD flags);
//void transport_modemstatus_clr(TDevice * dev, DWORD flags);
char * transport_decode_posix_error(int errcode);
//void transport_writeAsync(TDevice * dev, struct TNetPacket * frame, DWORD DriverDeviceHandle, TDriverSendFlags flags);
//int (*transport_RegisterDevice)(TDevice * newdev);

/**************************************************************************
***** Global Variables  ***************************************************
***************************************************************************/

static int transport_dBytesReadTotal = 0;

/**************************************************************************
***** Global Constants ****************************************************
**************************************************************************/

static BYTE bPowerlineSyncPre []={0xaa,0xaa};
static BYTE bPowerlineSyncPost[]={0x55,0x55};


/**************************************************************************
***** MACROS  *************************************************************
***************************************************************************/

#if 0
#define SET_DTR(f)    transport_modemstatus_set (f,  TIOCM_DTR)
#define SET_RTS(f)    transport_modemstatus_set (f,  TIOCM_RTS)
#define CLR_RTS(f)    transport_modemstatus_clr (f,  TIOCM_RTS)
#define CLR_DTR(f)    transport_modemstatus_clr (f,  TIOCM_DTR)
#define IS_DCD_SET(f) transport_modem_get_status(f,  TIOCM_CD )
#endif

//#define CREATE_VAR_THIS(d,interface) interface this = (void*)((d)->priv)

#ifdef USING_POSIX_AIO
#undef USING_POSIX_AIO
#endif
#define USING_POSIX_AIO 0

#if 1 == USING_POSIX_AIO
void aio_completion_handler( sigval_t sigval );
#endif


/**************************************************************************
***** IMPLEMENTATION ******************************************************
***************************************************************************/




/**************************************************************************
   Description   : Open this device
   Parameter     : dev Instance pointer of Bus driver
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 17.08.2000, 1.0, Created
                   Pruessing, 12.09.2000, 1.1, erweitert
**************************************************************************/
SHARED_FUNCTION BOOL transport_open(TDevice * dev) {
//   int rate;
//   CREATE_VAR_THIS(dev, struct TSerialPosixPriv *);

   YASDI_DEBUG((VERBOSE_HWL,"transport_open('%s')\n",dev->cName));

   // is the device in right state ??
   if (dev->DeviceState == DS_ONLINE)
   {
      YASDI_DEBUG((VERBOSE_HWL,"transport_open: Device is already open!\n"));
      return TRUE;
   }

	if (dev->tp->open(dev->tp_handle)) goto transport_open_error;
#if 0
   //open transport port in non blocking mode
   this->fd = open( this->cPort, O_RDWR | O_NOCTTY | O_NDELAY );
   if (this->fd >= 0)
   {
      // don't block 
      fcntl(this->fd, F_SETFL, FNDELAY);

      // get current device options
      tcgetattr(this->fd, &options);

      // set the transport port in RAW Mode (non cooked)
      options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
      options.c_cflag |= (CLOCAL | CREAD);
      options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
      options.c_oflag &= ~OPOST;
      options.c_cc[VMIN]  = 0;
      options.c_cc[VTIME] = 5;

      //set the right baud rate
      switch(this->dBaudrate)
	   {
         case 57600:   rate = B57600;   break;
         case 38400:   rate = B38400;   break;
         case 19200:   rate = B19200;   break;
         case 9600:    rate = B9600;    break;
         case 4800:    rate = B4800;    break;
         case 2400:	  rate = B2400;    break;
         case 1200:    rate = B1200;    break;
         case 600:	  rate = B600;	    break;
         case 300:	  rate = B300;	    break;
         case 150:	  rate = B150;	    break;
         case 110:	  rate = B110;	    break;
         default:      rate = B1200;
	   }
      cfsetispeed(&options, rate);
      cfsetospeed(&options, rate);

      // 8 bits, no paraty, sone stop bit
      options.c_cflag &= ~PARENB;
      options.c_cflag &= ~CSTOPB;
      options.c_cflag &= ~CSIZE;
      options.c_cflag |= CS8;

      // no handshake 
      options.c_cflag &= ~CRTSCTS;
     
      // set new parameter
      tcsetattr(this->fd, TCSANOW, &options);

      //some transport ports seams to be mirical because 
      //you can open it but you cant use it (e.g. when an port is not available)
      //So after open it try to check for incomming data. If this fails the port is not ready
      //and can't be used.
      if (ioctl(this->fd, FIONREAD, &iBytesInBuffer) < 0)
      {
         YASDI_DEBUG((VERBOSE_WARNING, "transport: Unable to open transport port '%s' (ioctrl/FIOREAD had failed)\n", this->cPort));
         return FALSE;
      }
#endif

	dev->DeviceState = DS_ONLINE;
	return TRUE;
transport_open_error:
//	YASDI_DEBUG((VERBOSE_WARNING, "transport: Unable to open transport port '%s'\n", this->cPort));
	log_write(LOG_SYSERR,"smanet: unable to open transport");
	return FALSE;
}


/**************************************************************************
   Description   : Close Bus Driver
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void transport_release(TDevice * dev)
{
//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

	dprintf(1,"closing!\n");
	dev->tp->close(dev->tp_handle);
#if 0
   if (this->fd >=0)
   {
      close(this->fd);
      this->fd = -1;
   }
#endif

   /* device in state offline now */
   dev->DeviceState = DS_OFFLINE;
}


/**************************************************************************
   Description   : Wait until Powerline or RS485 Bus is free
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
void transport_wait_bus_free(TDevice * dev)
{
#if 0
   int iCnt = 0;
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);
   /*
    * Wait on RS485/Powerline from 85ms to 115ms
	* Get state of Line free from DCD line (RS485-Piggyback, SMAPowerline modem)
	*/
   if (SERMT_POWERLINE == this->media)
   {
      do
	   {
	     iCnt = 0;
		 while(IS_DCD_SET( dev ))
		 {
         // already wait 1000 ms
         if ( iCnt > 1005 )
			{
			   //The time for one transmission => 1000ms maximum
			   //If the line is more than 1000ms set, something is wrong. Maybe an SBC is connected
            YASDI_DEBUG((VERBOSE_HWL,"transport: Error in Bus Arbitration. Maybe an SunnyBoyControl is connected?\n"));
			   return;
			}

			//wait for silence 
		 	os_thread_sleep(5); 
            iCnt += 5;
		 }

		  // wait random time between 85 and 115ms
	     os_thread_sleep( 85 + os_rand(0,7)*5);
	  }
	  while(IS_DCD_SET( dev )); //is bus now free???
   }
#endif

   return;
}

#if 0
/**************************************************************************
   Description   : Prepare Bus for receiving data
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void transport_prepare_recv(TDevice * dev)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   switch(this->media)
   {
      case SERMT_RS485:
      case SERMT_POWERLINE:

         //HP: 26.10.07: FEP-Problem: RS485-Umschaltung fehlerhaft            
         tcdrain(this->fd);

         os_thread_sleep(5);
         CLR_RTS(dev);
         SET_DTR(dev);
         break;

      case SERMT_RS232:
	  default:
         break;
   }
}

/**************************************************************************
   Description   : Prepare Bus for sending data
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
void transport_prepare_send(TDevice * dev)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   switch( this->media )
   {
      case SERMT_POWERLINE:
      case SERMT_RS485:           
         SET_RTS(dev);  
         CLR_DTR(dev);  
         os_thread_sleep(5);
         break;

      case SERMT_RS232:
      default:
         break;
   }
}
#endif


/**************************************************************************
   Description   : Write data to bus
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Datetransport, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void transport_write(TDevice * dev, struct TNetPacket * frame, DWORD DriverDeviceHandle, TDriverSendFlags flags)
{
   int ires;
   DWORD dBytesSend = 0;

//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);


   //check if device is in wrong state
   if ( dev->DeviceState != DS_ONLINE )
   {
      YASDI_DEBUG((VERBOSE_HWL,"transport: Nothing send. Bus driver '%s' is offline.\n", dev->cName ));
	   return;
   }

#if 0
   //Wait until bus is free
   transport_wait_bus_free(dev);

   //prepare to send
   transport_prepare_send(dev);

   //On powerline mode set "sync mark" before and after packet 
   if (this->media == SERMT_POWERLINE)
   {
      TNetPacket_AddHead(frame, bPowerlineSyncPre ,sizeof(bPowerlineSyncPre));
      TNetPacket_AddTail(frame, bPowerlineSyncPost,sizeof(bPowerlineSyncPost));
   }
#endif

   // transmit all buffer fragments 
   startagain: 
   {
      BYTE * framedata=NULL;
      WORD framedatasize=0;
      FOREACH_IN_BUFFER(frame, framedata, &framedatasize)
      {
	bindump("put frame",framedata,framedatasize);
	ires = dev->tp->write(dev->tp_handle,framedata, framedatasize);
         if (ires < 0)
         {
            YASDI_DEBUG((VERBOSE_HWL, "transport_write: Write error: %d code = %s\n", 
                         ires, transport_decode_posix_error(errno) ));
            if (transport_reopen(dev)) goto startagain;
		break;
         }
	usleep(550000);
         dBytesSend += ires; 
      }
   }

  	/* calculate total send bytes */
//   this->dBytesSendTotal += dBytesSend;
   
   /* prepare to receive */
//   transport_prepare_recv(dev);
}



/**************************************************************************
*
* NAME        : transport_reopen
*
* DESCRIPTION : description
*
*
***************************************************************************
*
* IN     : struct TDevice * dev
*
* OUT    : ---
*
* RETURN : BOOL
*
* THROWS : ---
*
**************************************************************************/
BOOL transport_reopen(TDevice * dev)
{
	dprintf(1,"reopening!\n");
//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);
   //do not use the transport_release function because after this call the 
   //state is reseted to "not online"
   transport_release( dev );
   transport_open   ( dev );
   
   //the device MUST still marked as open...
   dev->DeviceState = DS_ONLINE;
   
   //the real result is an valid filedescriptor...
//   return this->fd >= 0;
	return 1;
} 


/**************************************************************************
   Description   : Read bytes form transport port as much as possible
   Parameter     : dev = Drivce Instance
                   DestBuffer = pointer to buffer to store bytes in
                   dBufferSize = max size of buffer
   Return-Value  : count of bytes read
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD transport_read(TDevice * dev, BYTE * DestBuffer, DWORD dBufferSize, DWORD * DriverDevHandle) {
//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);
   int BytesRead;

   if ( dev->DeviceState == DS_ONLINE )
   {
      int iBytesInBuffer;
      int ires;
      
#if 0
      // is there something to read?
      ires = ioctl(this->fd, FIONREAD, &iBytesInBuffer);
      if (ires < 0)
      {
         YASDI_DEBUG((VERBOSE_HWL, "transport_read: Error checking for incoming data '%s'. errno=%s\n",
                   this->cPort, transport_decode_posix_error(errno) )); 
         if (!transport_reopen(dev)) return 0;  
      }
      if (iBytesInBuffer == 0) return 0;
	
      //limit the read for the maximum destination buffer size
      dBufferSize = min( (int)dBufferSize, iBytesInBuffer);
#endif

      //Read now 
//      BytesRead = read(this->fd, DestBuffer, dBufferSize );
	BytesRead = dev->tp->read(dev->tp_handle,DestBuffer,dBufferSize);
      if (BytesRead < 0)
      {
	log_write(LOG_SYSERR,"error readinig from transport");
         BytesRead = 0;
         transport_reopen(dev);
      }
      else
	   {
         transport_dBytesReadTotal += BytesRead;   
         //YASDI_DEBUG((VERBOSE_HWL, "transport_read: Read %d bytes from transport port %s\n", BytesRead, this->cPort ));
      }
	  
	  //print received bytes for debug...
#if 0
		{
			int i;
			if (BytesRead) YASDI_DEBUG((VERBOSE_HWL, "Received Data:\n"));
			for(i = 0 ; i < BytesRead; i++)
			{
				YASDI_DEBUG((VERBOSE_HWL, "[%2x]", DestBuffer[i]));
			}
			if (BytesRead) YASDI_DEBUG((VERBOSE_HWL, "\n"));
		}
#endif
		
	}
	else
	{
		YASDI_DEBUG((VERBOSE_HWL,"transport_read: Can't read because driver '%s' is not online.\n", dev->cName));
		BytesRead = 0;
	}

	return BytesRead;
}



#if 0
/**************************************************************************
   Description   : Setting of modem status bits
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
void transport_modemstatus_set(TDevice * dev, DWORD flags)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   DWORD status;
   ioctl(this->fd, TIOCMGET, &status);
   status |= flags;
   ioctl(this->fd, TIOCMSET, &status);
}


/**************************************************************************
   Description   : Clear of modem status bits
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
void transport_modemstatus_clr(TDevice * dev, DWORD flags)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   DWORD status;
   ioctl(this->fd, TIOCMGET, &status);
   status &= ~flags;
   ioctl(this->fd, TIOCMSET, &status);
}

DWORD transport_modem_get_status(TDevice * dev, DWORD flags)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);
   DWORD status;

   ioctl(this->fd, TIOCMGET, &status);

  	return (status & flags);
}
#endif

/**************************************************************************
   Description   : Return the "Maximum Transmit Unit" of this
                   bus device. Note that with Powerline you should only send
                   one second and that's why the MTU is much smaller
                   than on RS232/RS485.

   Parameter     : MTU of this driver
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.10.2001, 1.0, Created
**************************************************************************/
int transport_GetMTU(TDevice * dev)
{
//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

#if 0
   switch(this->media)
   {
      case SERMT_POWERLINE: return 100; break;
      case SERMT_RS232:
      case SERMT_RS485:
      default:              return 255; break;
   }
#endif
	dprintf(7,"returning 255\n");
	return 255;
}


/**************************************************************************
   Description   : Destructor of the bus driver
   Parameter     : dev = Zeiger auf Treiber-Instanz
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.04.2001, 1.0, Created
**************************************************************************/
void transport_destroy(TDevice * dev)
{
#if 0
//   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   #if (1 == USING_POSIX_AIO)
   //Free the packet buffer
   TNetPacketManagement_FreeBuffer( this->sendframe );
   #endif

   assert( this );
   free( this );
#endif
}

/**************************************************************************
*
* NAME        : transport_decode_posix_error
*
* DESCRIPTION : decode an posix error code (from var "errno")
*
*
***************************************************************************
*
* IN     : int errcode
*
* OUT    : ---
*
* RETURN : char *
*
* THROWS : ---
*
**************************************************************************/
char * transport_decode_posix_error(int errcode)
{
   char * code = NULL;
   #ifdef DEBUG
   switch(errcode)
   {
      case EBADF:  code = "EBADF";   break; 
      case EFAULT: code = "EFAULT";  break; 
      case ENOTTY: code = "ENOTTY";  break; 
      case EINVAL: code = "EINVAL";  break; 
      case EFBIG:  code = "EFBIG";   break; 
      case EINTR:  code = "EINTR";   break; 
      case ENOSPC: code = "ENOSPC";  break; 
      case EIO:    code = "EIO";     break; 
      default:     code = "default"; break; 
   }
   #endif   
   return code;
}

/**************************************************************************
   Description   : Device constructor: Create ONE instance of device
   Parameter     : Unit (for identification in profile section)
   Return-Value  : Pointer to instance or zero
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
**************************************************************************/
TDevice * transport_create(smanet_session_t *s) {
	TDevice * interface;
//	struct TSerialPosixPriv * priv;
//	char transport[32],target[64],topts[64],*p;

	dprintf(1,"creating interface...\n");
#if 0
   char cDefaultDev[10];
   char cMedia[20];
   char cConfigPath[50];
#endif


   /*
   ** Device-Struktur und private Struktur initialisieren
   */
   interface = (void*)malloc(sizeof( TDevice ));
//   priv      = (void*)malloc(sizeof( struct TSerialPosixPriv ));

//   if (interface && priv)
   if (interface) {
      /*
       * init driver structur
	   */
      memset(interface, 0,sizeof(TDevice));
//      memset(priv,0, sizeof(struct TSerialPosixPriv));
//      interface->priv = priv;
      interface->DeviceState = DS_OFFLINE;
//      priv->fd = -1;
//      priv->dBytesSendTotal = 0;

      #if (1 == USING_POSIX_AIO)
      //Init the posix async IO request struct for sending...
      priv->aiocbp = (void*)malloc( sizeof(struct aiocb) );
      if ( priv->aiocbp )
      {
         memset( priv->aiocbp ,0, sizeof(struct aiocb) );
      }
      //We need one packet buffer (this will be sended async)
      priv->sendframe = TNetPacketManagement_GetPacket();
      #endif


      /*
      ** init driver privates
      */
      interface->Open      = transport_open;
      interface->Close     = transport_release;
      interface->Write     = transport_write; 
      interface->Read      = transport_read;
      interface->GetMTU    = transport_GetMTU;

	interface->tp = s->tp;
	interface->tp_handle = s->tp_handle;

#if 0
	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	if (!strlen(transport) || !strlen(target)) {
		log_write(LOG_ERROR,"transport and target must be specified\n");
		return 0;
	}
	interface->tp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
	if (!interface->tp) {
		log_write(LOG_ERROR,"error loading transport %s\n", transport);
		return 0;
	}
	interface->tp_handle = interface->tp->new(0,target,topts);
	if (!interface->tp_handle) {
		log_write(LOG_ERROR,"error initializing %s\n", transport);
		return 0;
	}
#endif
	strcpy(interface->cName,interface->tp->name);
#if 0
	p = cfg_get_item(cfg, "smanet","media");
	if (!p) p = "RS232";
      if (strcasecmp(p,"RS232")==0)
      	 priv->media = SERMT_RS232;
      if (strcasecmp(p,"RS485")==0)
         priv->media = SERMT_RS485;
      if (strcasecmp(p,"Powerline")==0)
         priv->media = SERMT_POWERLINE;
      YASDI_DEBUG((VERBOSE_HWL, "Media = '%s'\n",p));
#endif

#if 0
      //Using Posix AIO for sending?
      #if (1 == USING_POSIX_AIO)
      interface->Write     = transport_writeAsync;
      #endif

      sprintf(interface->cName,"COM%lu", (unsigned long)dUnit); // bus driver name


      /*
       * Read Settings from configuration
       */
      sprintf(cConfigPath,"%s.Device", interface->cName);

      TRepository_GetElementStr(cConfigPath, cDefaultDev, priv->cPort, sizeof(priv->cPort) );
      YASDI_DEBUG((VERBOSE_HWL, "Device = '%s'\n",priv->cPort));

      /* Baudrate */
      sprintf(cConfigPath,"%s.Baudrate", interface->cName);
      priv->dBaudrate = TRepository_GetElementInt( cConfigPath, 19200 );
      YASDI_DEBUG((VERBOSE_HWL, "Baudrate = %ld\n", priv->dBaudrate));

      /* Medium: RS232, RS485 or Powerline */
      cMedia[0]=0;
      sprintf(cConfigPath,"%s.Media", interface->cName);
      TRepository_GetElementStr(cConfigPath, "Rs232", cMedia, sizeof(cMedia) );
      if (strcasecmp(cMedia,"RS232")==0)
      	 priv->media = SERMT_RS232;
      if (strcasecmp(cMedia,"RS485")==0)
         priv->media = SERMT_RS485;
      if (strcasecmp(cMedia,"Powerline")==0)
         priv->media = SERMT_POWERLINE;
      YASDI_DEBUG((VERBOSE_HWL, "Media = '%s'\n",cMedia));
#endif

      /*
      ** register this new bus device driver in the YASDI core
      */
	dprintf(1,"registering device!\n");
//	(*transport_RegisterDevice)( interface );
	s->regfunc(interface);
   }

   return interface;
}


/**************************************************************************
   Description   : Erzeugt alle Instanzen des Treibers laut den
                   Einstellungen im Profile '$HOME/.transportprofile'
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.04.2001, 1.0, Created
**************************************************************************/
void transport_create_all(smanet_session_t *s)
{
	transport_create(s);
#if 0
   int i;
   TDevice * dev = NULL;

   /*
    * Try to open all driver (read it from configuration)
   ** (COM1 - COM255)
   */
   for(i=0;i<255;i++)
   {
      char ConfigPath[50];
      sprintf(ConfigPath,"COM%d.Device",i);
      // profile exists?
      if (TRepository_GetIsElementExist( ConfigPath))
      {
         dev = transport_create(i);
	 UNUSED_VAR(dev);
      }
   }
#endif
}


/**************************************************************************
   Description   : Init driver module
   Parameter     : ---
   Return-Value  : == 0 => ok
                   != 0 => Fehler
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
                   Pruessing, 23.06.2001, 1.1, Uebergabe von Repository und
                                               Registrierungsfunktion per
                                               Parameter
**************************************************************************/
//int InitYasdiModule( void * RegFuncPtr, TOnDriverEvent eventCallback )
//int transport_init( void * RegFuncPtr, TOnDriverEvent eventCallback )
int transport_init( smanet_session_t *s ) {

   /* store functions for registration... */
//   transport_RegisterDevice = RegFuncPtr;
//	s->regfunc = RegFuncPtr;

   /*
   ** Create all drivers
   */
   transport_create_all(s);

   return 0; /* 0 => ok */
}


/**************************************************************************
   Description   : Deinitialisieren des Moduls
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
**************************************************************************/
#if 0
void CleanupYasdiModul(void)
{
   YASDI_DEBUG((VERBOSE_HWL,"Serial POSIX Driver: bye bye...\n"));
}
#endif

#if (1 == USING_POSIX_AIO)
/**************************************************************************
   Description   : Send to transport port using "Posix Async IO"
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.02.2009, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void transport_writeAsync(TDevice * dev, struct TNetPacket * frame, DWORD DriverDeviceHandle, TDriverSendFlags flags)
{
   CREATE_VAR_THIS(dev,struct TSerialPosixPriv *);

   //check if device is in wrong state
   if ( dev->DeviceState != DS_ONLINE )
   {
      YASDI_DEBUG((VERBOSE_HWL,"Serial(async): Nothing send. Bus driver '%s' is offline.\n", dev->cName ));
	   return; 
   }

   //an old running async io request must be ended first. 
   //So block here until this is finished.
   //Therefore this implementation can only send ONE packet async
   {
      #define ASYNC_LIST_MAX 1
      struct aiocb * cblist[ASYNC_LIST_MAX];
      cblist[0] = this->aiocbp;
      YASDI_DEBUG((VERBOSE_HWL,"Serial(async): Async Suspend..... %s\n", dev->cName ));
      aio_suspend( cblist, ASYNC_LIST_MAX, NULL );
      YASDI_DEBUG((VERBOSE_HWL,"Serial(async): Async Suspend done..... %s\n", dev->cName ));
   }

   //On powerline mode set "sync mark" before and after packet 
   if (this->media == SERMT_POWERLINE)
   {
      TNetPacket_AddHead(frame, bPowerlineSyncPre ,sizeof(bPowerlineSyncPre));
      TNetPacket_AddTail(frame, bPowerlineSyncPost,sizeof(bPowerlineSyncPost));
   }


   //Das Paket muss hier derzeit kopiert werden, da wird sync senden und
   //der Aufrufer davon derzeit ausgeht, dass das Paket gesendet wurde...
   TNetPacket_Clear( this->sendframe );
   TNetPacket_Copy(this->sendframe, frame);   

   //Wait until bus is free
   transport_wait_bus_free(dev);

   //prepare to send
   transport_prepare_send(dev);


   // transmit all buffer fragments. Warning: work only if the packet contains
   // only one fragment!
   {
      BYTE * framedata = NULL;
      WORD framedatasize = 0;
      FOREACH_IN_BUFFER(this->sendframe, framedata, &framedatasize)
      {
         //Schreibe async...
         this->aiocbp->aio_fildes = this->fd;
         this->aiocbp->aio_buf    = framedata;
         this->aiocbp->aio_nbytes = framedatasize;

         //we wahnt to called by an callback thread...
         this->aiocbp->aio_sigevent.sigev_notify                = SIGEV_THREAD;
         this->aiocbp->aio_sigevent.sigev_notify_function       = aio_completion_handler;
         this->aiocbp->aio_sigevent.sigev_notify_attributes     = NULL;
         this->aiocbp->aio_sigevent.sigev_value.sival_ptr       = dev; //instance pointer to callback function

         //write async...
         if (0 > aio_write( this->aiocbp ))
         {
            YASDI_DEBUG((VERBOSE_HWL, "transport_write: Write error for '%s'\n", dev->cName));
         }
         else
         {
            YASDI_DEBUG((1, "transport_write: async write started for '%s'\n", dev->cName ));
         }
      }
   }
}

//
                                       
/**************************************************************************
   Description   : Callback handler: Called when async write was finished...
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.02.2009, 1.0, Created
**************************************************************************/
void transport_aio_completion_handler( sigval_t sigval )
{
   TDevice * dev = (TDevice*)sigval.sival_ptr;
   struct TSerialPosixPriv * this = (struct TSerialPosixPriv*)dev->priv;
   struct aiocb * req = this->aiocbp;

   YASDI_DEBUG((1, "transport: transport_aio_completion_handler called '%s'\n", dev->cName ));

   /* Did the request complete? */
   if (aio_error( req ) == 0) 
   {
      YASDI_DEBUG((1, "transport: aio_write completed successfully for '%s'\n", dev->cName ));

      // Request completed successfully, get the return status 
      aio_return( req );
   }
   else
   {
      YASDI_DEBUG((1, "aio_write NOT completed!\n"));
   }

   /* all sended: prepare to receive */
   transport_prepare_recv(dev);

   /* calculate total send bytes */
   this->dBytesSendTotal += req->aio_nbytes;

   YASDI_DEBUG((1, "transport: transport_aio_completion_handler ended '%s'\n", dev->cName ));
   return;
}
#endif
