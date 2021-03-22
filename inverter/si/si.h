
#ifndef __SOLARD_SI_H
#define __SOLARD_SI_H

#include "agent.h"
#include <linux/can.h>

struct si_session {
	pthread_t th;
	/* 0x300 to 0x30F */
	struct can_frame frames[16];
	uint32_t bitmap;
	solard_module_t *can;
	void *can_handle;
	solard_module_t *serial;
	void *serial_handle;
	int (*get_data)(struct si_session *, int id, uint8_t *data, int len);
	uint16_t state;
};
typedef struct si_session si_session_t;

#define SI_STATE_RUNNING 0x01
#define SI_STATE_OPEN 0x10

void *si_recv_thread(void *handle);
int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int len);
int si_read(si_session_t *,void *,int);
int si_write(si_session_t *,void *,int);
int si_config(void *,char *,char *,list);

#define si_getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = (int)(v); }

#endif /* __SOLARD_SI_H */
