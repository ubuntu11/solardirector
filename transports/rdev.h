
#ifndef __SD_RDEV_H
#define __SD_RDEV_H

#ifdef WINDOWS
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
#endif

struct __attribute__((packed, aligned(1))) rdev_header {
        union {
                uint8_t opcode;
                uint8_t status;
        };
        uint8_t unit;
        uint16_t control;
        uint16_t len;
};
typedef struct rdev_header rdev_header_t;

int rdev_send(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, uint16_t data_len);
int rdev_recv(socket_t fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout);
int rdev_request(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len);

#define RDEV_NAME_LEN 32
#define RDEV_TYPE_LEN 16

/* Functions */
enum RDEV_OPCODE {
        RDEV_OPCODE_OPEN,
        RDEV_OPCODE_CLOSE,
        RDEV_OPCODE_READ,
        RDEV_OPCODE_WRITE,
	RDEV_OPCODE_MAX
};

#define RDEV_STATUS_SUCCESS 0
#define RDEV_STATUS_ERROR 1

#endif /* __SD_RDEV_H */
