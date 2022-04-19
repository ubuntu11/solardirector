
#include "transports.h"
#include "can.h"
#include "debug.h"

extern int debug;

int get(solard_driver_t *tp, void *h, int id, can_frame_t *frame) {
	uint32_t can_id;
	int bytes;

	can_id = id;
	bytes = tp->read(h,&can_id,frame,sizeof(*frame));
	dprintf(1,"bytes: %d\n", bytes);
	if (bytes < 0) return 1;
//	bindump("frame",&frame,sizeof(frame));
	return 0;
}

void dump32(solard_driver_t *tp, void *h, int id) {
	can_frame_t frame;
	uint8_t *data = frame.data;
	int n1,n2;

	if (get(tp,h,id,&frame)) return;
	n1 = *((int *)&data[0]);
	n2 = *((int *)&data[4]);
	printf("%03x n1: %d, n2: %d\n", id, n1, n2);
}

void dump16(solard_driver_t *tp, void *h, int id) {
	can_frame_t frame;
	uint8_t *data = frame.data;
	short n1,n2,n3,n4;

	if (get(tp,h,id,&frame)) return;
	n1 = *((short *)&data[0]);
	n2 = *((short *)&data[2]);
	n3 = *((short *)&data[4]);
	n4 = *((short *)&data[6]);
	printf("%03x n1: %d, n2: %d, n3: %d, n4: %d\n", id, n1, n2, n3, n4);
}

int main(void) {
	solard_driver_t *tp;
	void *h;
	can_frame_t frame;
	uint8_t *data = frame.data;
	float p1,p2,p3,p4;
	float a1,a2,a3,a4;

debug = 0;
log_open("dsi",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);
	tp = &can_driver;
//	h = tp->new("can0","500000,0x000-0x30F,yes");
	h = tp->new("can0","500000,,yes");
	printf("h: %p\n", h);
	if (!h) return 1;
	if (tp->open(h)) return 1;
	while(1) {
		get(tp,h,0x051,&frame);
		p1 = (float) *((short *)&data[0]) / 10;
		p1 *= (-1);
		a1 = (float) *((short *)&data[2]) * 10;
		get(tp,h,0x1E1,&frame);
		p2 = (float) *((int *)&data[0]);
		a2 = (float) *((int *)&data[4]);
		get(tp,h,0x1E2,&frame);
		p3 = (float) *((int *)&data[0]);
		a3 = (float) *((int *)&data[4]);
		get(tp,h,0x1E3,&frame);
		p4 = (float) *((int *)&data[0]);
		a4 = (float) *((int *)&data[4]);
		printf("p1: %.1f, p2: %.1f, p3: %.1f, p4: %.1f\n", p1,p2,p3,p4);
		printf("total: %.1f\n", p1+p2+p3+p4);
		printf("a1: %.1f, a2: %.1f, a3: %.1f, a4: %.1f\n", a1,a2,a3,a4);
#if 0
		dump32(tp,h,0x181);
		dump32(tp,h,0x182);
		dump32(tp,h,0x183);
		dump32(tp,h,0x1E1);
		dump32(tp,h,0x1E2);
		dump32(tp,h,0x1E3);
		dump32(tp,h,0x201);
		dump32(tp,h,0x202);
		dump32(tp,h,0x203);
		dump16(tp,h,0x300);
		dump16(tp,h,0x302);
#endif
		sleep(1);
//		break;
	}
	tp->close(h);
}
