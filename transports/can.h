
#ifndef __SD_CAN_H
#define __SD_CAN_H

#include <sys/types.h>
#ifdef WINDOWS
typedef uint32_t canid_t;
#define CAN_MAX_DLEN 8
struct can_frame {
	canid_t can_id;		/* 32 bit CAN_ID + EFF/RTR/ERR flags */
	uint8_t can_dlc;	/* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	uint8_t data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};
#if 0
struct can_frame {
	unsigned long can_id;
        unsigned char can_dlc;
        unsigned char data[8];
};
#endif
#else
#include <linux/can.h>
#endif

#endif
