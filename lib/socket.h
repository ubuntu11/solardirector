
#ifndef __SDSOCKET_H
#define __SDSOCKET_H

/*************************************************************************
 *
 * Socket and network functions
 *
 *************************************************************************/

/* Define the socket types */
enum SOCKET_TYPES {
	SOCKET_TYPE_TCP,		/* TCP socket */
	SOCKET_TYPE_UDP			/* UDP socket */
};
#define MAX_UDP_PACKET_SIZE	8192

struct SocketPortInfo {
	char name[32];
	char addr[16];
	unsigned short port;
};
#define PORTINFO_SIZE sizeof(struct SocketPortInfo)
typedef struct SocketPortInfo PORTINFO;

/* Define the Socket Info Structure */
struct SocketInfo {
#ifdef _MS_WINDOWS_
	unsigned int s;			/* Actual socket fd */
#else
	int s;				/* Actual socket fd */
#endif
	char buffer[8192];		/* read buffer */
	char *sptr,*eptr;		/* start and end buffer ptrs */
	PORTINFO local;			/* Local port info */
	PORTINFO remote;		/* Remote port info */
	int type;			/* Socket type */
	int timeout;			/* Recv timeout */
	int in;				/* Total bytes read */
	int out;			/* Total bytes written */
};
typedef struct socket_info socket_t;
typedef struct SocketInfo * SOCK;

/* Define the args to settos */
#define SOCKET_LOWDELAY		0	/* Low delay interactive socket */
#define SOCKET_THROUGHPUT	1	/* High-throughput data socket */

/* Define the socket set structure */
#define SOCKSET_MAX_SOCKETS		32
struct SocketSetInfo {
//	unsigned char fdset[SOCKSET_MAX_SOCKETS];
	unsigned long fdset[SOCKSET_MAX_SOCKETS];
#ifdef _MS_WINDOWS_
	unsigned int maxfd;
#else
	int maxfd;
#endif
};
#define SOCKSET_SIZE sizeof(struct SocketSetInfo)
typedef struct SocketSetInfo SOCKSET;

/* Define the network server argument block */
struct _netserver_args {
	SOCK s;				/* Socket */
	void *args;			/* Argument ptr from info block */
};
typedef struct _netserver_args NETSERVER_ARGS;
#define NETSERVER_ARGS_SIZE sizeof(struct _netserver_args)

/* Define the network server info block */
struct _netserver_info {
	unsigned short access_port;	/* Access port */
	int reuse_port;			/* Re-use port flag */
	int (*inetd_access)(SOCK,void *);/* INETD Access function (TCP only) */
	int (*tcp_access)(SOCK,void *);/* TCP Access function */
	int (*udp_access)(SOCK,void *);/* UDP Access function */
	void *access_args;		/* Args to pass access function(s) */
	unsigned short control_port;	/* Control port */
	int (*tcp_control)(SOCK,void *);/* TCP Control function */
	int (*udp_control)(SOCK,void *);/* UDP Control function */
	void *control_args;		/* Args to pass control functions(s) */
	int timeout;			/* # of secs to wait for connection */
	void (*timeout_func)(void *);	/* Func to call when timeout happens */
	void *timeout_args;		/* Args to pass timeout function */
	int (*startup)(void);		/* Func to call when starting up */
	int *terminate;			/* Ptr to terminate flag */
	void (*shutdown)(void);		/* Func to call when shutting down */
	void (*nologin)(SOCK,char *); /* Func to call when server down */
};
typedef struct _netserver_info NETSERVER;
#define NETSERVER_SIZE sizeof(struct _netserver_info)

/* Define the network functions */
#ifdef __cplusplus
extern "C" {
#endif
char *socket_getname(char *);
char *socket_getaddr(char *);
int socket_isaddr(char *);
int socket_getlocal(SOCK);
SOCK socket_dup(SOCK);
SOCK socket_open(int);
int socket_close(SOCK);
int socket_connect(SOCK,char *,unsigned short);
int socket_connectbyname(SOCK,char *,unsigned short);
SOCK socket_get_inetd(void);
int socket_reuse(SOCK);
int socket_reserve(SOCK,unsigned short);
int socket_recv(SOCK,void *,unsigned short);
int socket_send(SOCK,PORTINFO *,void *,unsigned short);
void socket_newset(SOCKSET *);
void socket_add2set(SOCKSET *,SOCK);
int socket_isset(SOCKSET *,SOCK);
int socket_chkset(SOCKSET *,int);
int socket_ready(SOCK,int);
SOCK socket_accept(SOCK);
int socket_timeout(SOCK,int);
int socket_settos(SOCK,int);
int socket_read(SOCK,void *,unsigned short);
int socket_write(SOCK,void *,unsigned short);
unsigned short socket_htons(unsigned short);
unsigned short socket_ntohs(unsigned short);
unsigned long socket_htonl(unsigned long);
unsigned long socket_ntohl(unsigned long);
int socket_printf(SOCK,char *,...);
int socket_gets(SOCK,char *,unsigned short);
int socket_telgets(SOCK,char *,unsigned short,int);
int send_packet(SOCK,PORTINFO *,void *,int,int);
int recv_packet(SOCK,PORTINFO *,void *,int);
int netserver(NETSERVER *);
#ifdef __cplusplus
}
#endif

#endif
