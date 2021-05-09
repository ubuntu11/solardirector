
#define DEBUG 1

#include "debug.h"

#include "../../transports/ip.c"
#include "../../transports/serial.c"

int main(void) {
	ip_session_t *is;
	serial_session_t *ss;
	uint8_t data[128];
	int i,bytes,retries;

	printf("starting...\n");
	debug = 9;
#if 1
	ss = serial_new(0,"/dev/ttyUSB0","115200");
	if (!ss) return 1;
	if (serial_open(ss)) return 1;
	while(1) {
	i = 0;
	data[i++] = 0x55;
	data[i++] = 0xAA;
	data[i++] = 0x01;
	data[i++] = 0xFF;
	data[i++] = 0x00;
	data[i++] = 0x00;
	data[i++] = 0xFF;
		serial_write(ss,data,7);
		retries=3;
		while(retries--) {
			sleep(1);
			bytes = serial_read(ss,data,sizeof(data));
			if (bytes < 0) return 1;
			if (!bytes) continue;
			bindump("S2 data",data,bytes);
			break;
		}
	}
#else
	is = ip_new(0,"192.168.1.210","");
	if (!is) return 1;
	if (ip_open(is)) return 1;
	while(1) {
	i = 0;
	data[i++] = 0x55;
	data[i++] = 0xAA;
	data[i++] = 0x01;
	data[i++] = 0xFF;
	data[i++] = 0x00;
	data[i++] = 0x00;
	data[i++] = 0xFF;
		if (ip_write(is,data,7) < 0) return 1;
		retries=3;
		while(retries--) {
			sleep(1);
			bytes = ip_read(is,data,sizeof(data));
			if (bytes < 0) return 1;
			if (!bytes) continue;
			bindump("data",data,bytes);
		}
	}
#endif
}
