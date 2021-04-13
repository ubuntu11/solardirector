
#include <string.h>
#include <sys/types.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include "socket.h"
#include "utils.h"
#include "debug.h"

#define dlevel 3

/* Get a name for an addr */
char *socket_getname(char *addr) {
	static char name[128];
	struct hostent *he;
	unsigned long net_addr;

	DLOG(LOG_DEBUG,"socket_getname: getting name for addr: %s",addr);
	name[0] = 0;
	net_addr = inet_addr(addr);
	he = gethostbyaddr((char *)&net_addr,sizeof(net_addr),AF_INET);
	if (he)
		strncat(name,he->h_name,sizeof(name)-1);
	else {
		DLOG(LOG_DEBUG|LOG_ERROR,"socket_getname: h_errno: %d",
			h_errno);
		strncat(name,addr,sizeof(name)-1);
	}
	DLOG(LOG_DEBUG,"socket_getname: name: %s",name);
	return name;
}

/* Get an addr for a name */
char *socket_getaddr(char *name) {
	static char addr[128];
	struct hostent *he;
	register unsigned char *ptr;

	DLOG(LOG_DEBUG,"socket_getaddr: getting addr for name: %s",name);
	addr[0] = 0;
	he = gethostbyname(name);
	if (he) {
		ptr = (unsigned char *) he->h_addr;
		sprintf(addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	} else {
		DLOG(LOG_DEBUG|LOG_ERROR,"socket_getaddr: h_errno: %d", h_errno);
		strncat(addr,name,sizeof(addr)-1);
	}
	DLOG(LOG_DEBUG,"socket_getaddr: addr: %s",addr);
	return addr;
}

/* Check if string is an IPv4 addr */
int socket_isaddr(char *string) {
	register char *ptr;
	int dots,digits;

	DLOG(LOG_DEBUG,"socket_isaddr: string: %s",string);

	digits = dots = 0;
	for(ptr=string; *ptr; ptr++) {
		if (*ptr == '.') {
			if (!digits) goto socket_isaddr_error;
			if (++dots > 4) goto socket_isaddr_error;
			digits = 0;
		} else if (isdigit((int)*ptr)) {
			if (++digits > 3) goto socket_isaddr_error;
		}
	}
	DLOG(LOG_DEBUG,"digits: %d, dots: %d\n", digits, dots);

	return 0;
//	DLOG(LOG_DEBUG,"socket_isaddr: tuples: %d", dots);

//	return (dots == 4 ? 1 : 0);
socket_isaddr_error:
	return 1;
}

int socket_getlocal(SOCK sp) {
	struct sockaddr_in sin;
	socklen_t sin_size = sizeof(sin);
	register unsigned char *ptr;

	if (!sp) return 1;
	sp->local.name[0] = sp->local.addr[0] = 0;
	if (getsockname(sp->s,(struct sockaddr *)&sin,&sin_size) == 0) {
		ptr = (unsigned char *) &sin.sin_addr.s_addr;
		sprintf(sp->local.addr,"%d.%d.%d.%d",
			ptr[0],ptr[1],ptr[2],ptr[3]);
	}
	strncat((char *)sp->local.name,socket_getname(sp->local.addr),
		sizeof(sp->local.name)-1);
	sp->local.port = ntohs(sin.sin_port);
	DLOG(LOG_DEBUG,"socket_getlocal: local info:");
	DLOG(LOG_DEBUG,"\tname: %s",sp->local.name);
	DLOG(LOG_DEBUG,"\taddr: %s",sp->local.addr);
	DLOG(LOG_DEBUG,"\tport: %d",sp->local.port);
	return 0;
}

/* Internal function to allocate and initalize a socket info block */
static SOCK _newsp(int s) {
	SOCK sp;

	/* Allocate mem for the socket pointer (sp) */
	sp = (SOCK) calloc(1,sizeof(*sp));
	if (!sp) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket: _newsp: malloc");
		return 0;
	}
	sp->s = s;
	sp->sptr = sp->eptr = sp->buffer;

	return sp;
}

/* Open a new socket */
SOCK socket_open(int type) {
	SOCK sp;
#ifdef _MS_WINDOWS_
	unsigned int s;
#else
	int s;
#endif
	int socket_type,proto_type;

	DLOG(LOG_DEBUG,"socket_open: type: %d",type);
 
	/* Set the type */
	if (type == SOCKET_TYPE_TCP) {
		socket_type = SOCK_STREAM;
		proto_type = IPPROTO_TCP;
	}
	else {
		socket_type = SOCK_DGRAM;
		proto_type = IPPROTO_UDP;
	}

	/* XXX Why would I use the proto??? */
	/* Open up a socket */
	s = socket(AF_INET,socket_type,proto_type);
#ifdef _MS_WINDOWS_
	if (s == SOCKET_ERROR) return 0;
#else
	if (s < 0) return 0;
#endif

	DLOG(LOG_DEBUG,"socket_open: fd: %u",s);

	/* Create the socket pointer */
	sp = _newsp(s);
	if (!sp) {
		close(s);
		return 0;
	}
	sp->type = type;
	return sp;
}

/* Close a socket */
int socket_close(SOCK sp) {
	if (!sp) return 1;
	DLOG(LOG_DEBUG,"socket_close: fd: %u",sp->s);
#ifdef _MS_WINDOWS_
	closesocket(sp->s);
#else
	close(sp->s);
#endif
	free(sp);
	return 0;
}

/* Connect an open socket by hostname */
int socket_connectbyname(SOCK sp,char *host,unsigned short port) {
	DLOG(LOG_DEBUG,"socket_connectbyname: host: %s",host);
	return(socket_connect(sp,socket_getaddr(host),port));
}

/* Connect an open socket */
int socket_connect(SOCK sp,char *addr,unsigned short port) {
	struct sockaddr_in sin;
	int sin_size = sizeof(sin);
#ifdef SOCKET_INFO
	register unsigned char *ptr;
#endif

	DLOG(LOG_DEBUG,"socket_connect: fd: %d, addr: %s, port: %d", sp->s,addr,port);

	/* Set up the addr info */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_port = htons(port);

	/* Connect the socket */
	if (connect(sp->s,(struct sockaddr *)&sin,sin_size) < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket_connect: connect");
		return 1;
	}

#ifdef SOCKET_INFO
	/* XXX Save remote addr and port for a later call to socket_send */
	sp->remote.name[0] = sp->remote.addr[0] = 0;
	strncat(sp->remote.name,socket_getname(addr),sizeof(sp->remote.name)-1);
	strncat(sp->remote.addr,addr,sizeof(sp->remote.addr)-1);
	sp->remote.port = port;

	/* XXX Save local addr and port for informational purposes */
	sp->local.name[0] = sp->local.addr[0] = 0;
	if (getsockname(sp->s,(struct sockaddr *)&sin,&sin_size) == 0) {
		ptr = (unsigned char *) &sin.sin_addr.s_addr;
		sprintf(sp->local.addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	}
	strncat(sp->local.name,socket_getname(sp->local.addr),
		sizeof(sp->local.name)-1);
	sp->local.port = ntohs(sin.sin_port);

	DLOG(LOG_DEBUG,"socket_connect: local info:");
	DLOG(LOG_DEBUG,"\tname: %s",sp->local.name);
	DLOG(LOG_DEBUG,"\taddr: %s",sp->local.addr);
	DLOG(LOG_DEBUG,"\tport: %d",sp->local.port);
	DLOG(LOG_DEBUG,"socket_connect: remote info:");
	DLOG(LOG_DEBUG,"\tname: %s",sp->remote.name);
	DLOG(LOG_DEBUG,"\taddr: %s",sp->remote.addr);
	DLOG(LOG_DEBUG,"\tport: %d",sp->remote.port);
#endif
	return 0;
}

#ifdef _WIN32
static unsigned int get_socket_handle( void)
{
	HANDLE hSocket = GetStdHandle( STD_INPUT_HANDLE);
	if (hSocket == INVALID_HANDLE_VALUE)
	{
		log_write(LOG_ERROR,"No socket from stdin; error %d", GetLastError());
		return INVALID_SOCKET;
	}
	else
		return ( (unsigned int) hSocket);
}
#endif

/* Get a socket from inetd */
SOCK socket_get_inetd(void) {
	SOCK sp;
	struct sockaddr_in sin;
	socklen_t sin_size;
	int st;
#ifdef _MS_WINDOWS_
	unsigned int s;
#else
	int s;
#endif
	register unsigned char *ptr;

#ifdef vms
	s = socket(UCX$C_AUXS,0,0);
	if (s < 0) return 0;
#else
#ifdef _WIN32
	s = get_socket_handle();
	if (s == INVALID_SOCKET) return 0;
#else
	s = 0;
#endif
#endif

	/* Create a new socket pointer */
	DLOG(LOG_DEBUG,"socket_get_inetd: calling _newsp...");
	sp = _newsp(s);
	if (!sp) return 0;

	/* Fill in the remote portion and type */
	DLOG(LOG_DEBUG,"socket_get_inetd: getting socket options...");
	sin_size = sizeof(st);
	if (getsockopt(s,SOL_SOCKET,SO_TYPE,(char *)&st,&sin_size) < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket_get_inetd: getsockopt");
		return 0;
	}
	if (st == SOCK_STREAM)
		sp->type = SOCKET_TYPE_TCP;
	else
		sp->type = SOCKET_TYPE_UDP;

	/* XXX Save remote addr and port for a later call to socket_send */
	DLOG(LOG_DEBUG,"socket_get_inetd: getting peer name...");
	sp->remote.name[0] = sp->remote.addr[0] = 0;
	sin_size = sizeof(sin);
	if (getpeername(sp->s,(struct sockaddr *)&sin,&sin_size) == 0) {
		ptr = (unsigned char *) &sin.sin_addr.s_addr;
		sprintf(sp->remote.addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	}
	strncat(sp->remote.name,socket_getname(sp->remote.addr),
		sizeof(sp->remote.name)-1);
	sp->remote.port = sin.sin_port;

	/* XXX Save local addr and port for informational purposes */
	sp->local.name[0] = sp->local.addr[0] = 0;
	if (getsockname(sp->s,(struct sockaddr *)&sin,&sin_size) == 0) {
		ptr = (unsigned char *) &sin.sin_addr.s_addr;
		sprintf(sp->local.addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	}
	strncat(sp->local.name,socket_getname(sp->local.addr),
		sizeof(sp->local.name)-1);
	sp->local.port = ntohs(sin.sin_port);

	DLOG(LOG_DEBUG,"socket_get_inetd: local info:");
	DLOG(LOG_DEBUG,"\tname: %s",sp->local.name);
	DLOG(LOG_DEBUG,"\taddr: %s",sp->local.addr);
	DLOG(LOG_DEBUG,"\tport: %d",sp->local.port);
	DLOG(LOG_DEBUG,"socket_get_inetd: remote info:");
	DLOG(LOG_DEBUG,"\tname: %s",sp->remote.name);
	DLOG(LOG_DEBUG,"\taddr: %s",sp->remote.addr);
	DLOG(LOG_DEBUG,"\tport: %d",sp->remote.port);

	return sp;
}

/* Reuse a port # */
int socket_reuse(SOCK sp) {
	int arg = 1, error;

	error = (setsockopt(sp->s,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,
						sizeof(arg)) < 0 ? 1 : 0);
	if (error) log_write(LOG_SYSERR, "setsockopt");
	return 0;
}

/* Reserve a port */
int socket_reserve(SOCK sp,unsigned short port) {
	struct sockaddr_in sin;
	socklen_t sin_size = sizeof(sin);
#ifdef IP_PORTRANGE
	int arg;
#endif

	DLOG(LOG_DEBUG,"socket_reserve: port requested: %d",port);

	/* Set up the addr info */
	memset((char *)&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	if (strlen(sp->local.addr)) {
		DLOG(LOG_DEBUG,"socket_reserve: binding to addr: %s", sp->local.addr);
		sin.sin_addr.s_addr = inet_addr(sp->local.addr);
	} else
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = ntohs(port);

#ifdef IP_PORTRANGE
	if (port == 0) {
		/* Set a range for port = 0 */
		DLOG(LOG_DEBUG, "socket_reserve: setting default port range");
		arg = IP_PORTRANGE_DEFAULT;
		if (setsockopt(sp->s,IPPROTO_IP,IP_PORTRANGE,(char *)&arg,
		    sizeof(arg)) < 0) {
			DLOG(LOG_DEBUG|LOG_SYSERR, "socket_reserve: setsockopt");
			return 1;
		}
	}
#endif /* IP_PORTRANGE */

	/* Bind the port */
	if (bind(sp->s,(struct sockaddr *)&sin,sin_size) < 0) {
		log_write(LOG_ERROR,"socket_reserve: bind");
		return 1;
	}

	/* Get the bound port */
	if (getsockname(sp->s,(struct sockaddr *)&sin,&sin_size) < 0) {
		log_write(LOG_ERROR,"socket_reserve: getsockname");
		return 1;
	}

	DLOG(LOG_DEBUG,"socket_reserve: port reserved: %d", ntohs(sin.sin_port));
	sp->local.port = ntohs(sin.sin_port);

	/* Listen on the port? */
	if (sp->type == SOCKET_TYPE_TCP) {
		DLOG(LOG_DEBUG,"socket_reserve: listening...");
		if (listen(sp->s,SOMAXCONN) < 0)
			return 1;
	}

	return 0;
}

/* Set the recv timeout for a socket */
int socket_timeout(SOCK sp,int secs) {
	DLOG(LOG_DEBUG,"socket_timeout: fd: %d, secs: %d",sp->s,secs);
	sp->timeout = secs;
	return 0;
#if 0
	struct timeval tv;
	int level;

	DLOG(LOG_DEBUG,"socket_timeout: fd: %d, secs: %d",sp->s,secs);

	/* Set the timeout */
	tv.tv_sec = secs;
	tv.tv_usec = 0;

	/* Set the option */
	level = SOL_SOCKET;
	if (setsockopt(sp->s,level,SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket_timeout: setsockopt");
		return 1;
	}
#endif
	return 0;
}

int socket_settos(SOCK sp,int opt) {
#ifndef WIN32
	int on;

	DLOG(LOG_DEBUG,"socket_settos: opt: %d",opt);

	if (opt == SOCKET_LOWDELAY)
		on = IPTOS_LOWDELAY;
	else if (opt == SOCKET_THROUGHPUT)
		on = IPTOS_THROUGHPUT;
	else
		return 1;

	/* Set the option */
	if (setsockopt(sp->s,IPPROTO_IP,IP_TOS,(char *)&on,sizeof(on)) < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket_settos: setsockopt");
		return 1;
	}
	return 0;
#else
	return 1;
#endif
}

/* Create a new socket set */
void socket_newset(SOCKSET *set) {
	/* Clear the set */
	memset((char *)set,0,SOCKSET_SIZE);
	set->maxfd = 0;
	return;
}

/* Add a socket to a socket set */
void socket_add2set(SOCKSET *set,SOCK s) {
	FD_SET(s->s,(fd_set *)&set->fdset);
	if (s->s > set->maxfd) set->maxfd = s->s;
	return;
}

/* Check if a socket is ready for reading */
int socket_isset(SOCKSET *set,SOCK s) {
	return(FD_ISSET(s->s,(fd_set *)&set->fdset));
}

/* 
** Check if any sockets in the set are ready 
** timeout values are:
**	-1 = wait forever
**	0 = don't wait
** 	> 0 = # of seconds to wait
*/
int socket_chkset(SOCKSET *set,int timeout) {
	struct timeval t,*tptr;
	int numset;

	/* Set the timeval to timeout (or not) */
	if (timeout >= 0) {
		t.tv_usec = 0;
		t.tv_sec = timeout;
		tptr = &t;
	}
	else
		tptr = 0;

	/* Do a select on the socket */
	numset = select(set->maxfd+1,(fd_set *)&set->fdset,(fd_set *)0,
			(fd_set *)0,tptr);
	if (numset < 0) return 0;

	return numset;
}

/* Is a socket ready for reading? */
int socket_ready(SOCK s,int timeout) {
	fd_set r;
	struct timeval t,*tp;
	int count, ready;

	DLOG(LOG_DEBUG2,"socket_ready: socket: %d, timeout: %d", s->s,timeout);

	/* Set the timeval to timeout (or not) */
	if (timeout >= 0) {
		t.tv_usec = 0;
		t.tv_sec = timeout;
		tp = &t;
	}
	else
		tp = 0;

	/* Zero out the set */
	DLOG(LOG_DEBUG2,"socket_ready: zeroing set...");
	FD_ZERO(&r);

	/* Add the socket to the set */
	DLOG(LOG_DEBUG2,"socket_ready: adding socket...");
	FD_SET(s->s,&r);

	/* Do a select on the socket */
	DLOG(LOG_DEBUG2,"socket_ready: calling select...");
	do {
		count = select(FD_SETSIZE,(fd_set *)&r,(fd_set *)0,
			(fd_set *)0,tp);
	} while(count < 0 && errno == EINTR);
	DLOG(LOG_DEBUG2,"socket_ready: count: %d, r: %d",
		count,FD_ISSET(s->s,&r));
	if (count > 0 && FD_ISSET(s->s,&r))
		ready = 1;
	else
		ready = 0;
	DLOG(LOG_DEBUG2,"socket_ready: returning: %d", ready);
	return ready;
}

/* Send a packet (udp) */
int socket_send(SOCK sp,PORTINFO *info,void *buffer,unsigned short buflen) {
	struct sockaddr_in sin;
	int sin_size = sizeof(sin);

	/* Datagrams only, please */
	if (sp->type != SOCKET_TYPE_UDP) return 1;

	DLOG(LOG_DEBUG,"socket_send: sending %d bytes to %s(%s) port %d",
		buflen,info->name,info->addr,info->port);

	/* Set up the addr info */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(info->addr);
	sin.sin_port = htons(info->port);
	
	/* Send the packet */
	if (sendto(sp->s,buffer,buflen,0,(struct sockaddr *)&sin,sin_size) < 0)
		return 1;

	return 0;
}

/* Accept a connection (tcp) */
SOCK socket_accept(SOCK sp) {
	SOCK cp;
	struct sockaddr_in sin;
	socklen_t sin_size = sizeof(sin);
	int c;
	register unsigned char *ptr;

	/* Streams only, please */
	DLOG(LOG_DEBUG,"socket_accept: type: %d",sp->type);
	if (sp->type != SOCKET_TYPE_TCP) return 0;

	/* Accept a connection */
	DLOG(LOG_DEBUG,"socket_accept: accepting from fd: %d",sp->s);
	c = accept(sp->s,(struct sockaddr *)&sin,&sin_size);
	if (c < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"socket_accept: accept");
		return 0;
	}

	DLOG(LOG_DEBUG,"socket_accept: accepted fd: %d",c);
	cp = _newsp(c);
	if (!cp) return 0;
	cp->type = SOCKET_TYPE_TCP;

	/* Get the remote port info */
	ptr = (unsigned char *) &sin.sin_addr.s_addr;
	sprintf(cp->remote.addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	strncat(cp->remote.name,socket_getname(cp->remote.addr),
		sizeof(cp->remote.name)-1);
	cp->remote.port = ntohs(sin.sin_port);

	/* Get the local port info */
	cp->local.port = cp->local.addr[0] = cp->local.name[0] = 0;
	if (getsockname(c,(struct sockaddr *)&sin,&sin_size) == 0) {
		ptr = (unsigned char *) &sin.sin_addr.s_addr;
		sprintf(cp->local.addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		strncat(cp->local.name,socket_getname(cp->local.addr),
			sizeof(cp->local.name)-1);
		cp->local.port = ntohs(sin.sin_port);
	}

	DLOG(LOG_DEBUG,"socket_accept: local info:");
	DLOG(LOG_DEBUG,"\tname: %s",cp->local.name);
	DLOG(LOG_DEBUG,"\taddr: %s",cp->local.addr);
	DLOG(LOG_DEBUG,"\tport: %d",cp->local.port);
	DLOG(LOG_DEBUG,"socket_accept: remote info:");
	DLOG(LOG_DEBUG,"\tname: %s",cp->remote.name);
	DLOG(LOG_DEBUG,"\taddr: %s",cp->remote.addr);
	DLOG(LOG_DEBUG,"\tport: %d",cp->remote.port);

#ifdef BIG_BUFFER
	/* Set a nice big fat buffer */
	on = 65536;
	(void)setsockopt(c, SOL_SOCKET, SO_SNDBUF,(char *)&on,sizeof on);
#endif

	return cp;
}

/* Read from a socket */
int socket_read(SOCK sp,void *buffer,unsigned short bufsize) {
	register char *ptr = (char *) buffer;
	register int bytes_left = bufsize,count,total;

	DLOG(LOG_DEBUG2, "socket_read: reading %d bytes from socket %d...", bufsize,sp->s);
	total = 0;
	while(bytes_left) {
		count = recv(sp->s,ptr,bytes_left,0);
		DLOG(LOG_DEBUG3,"socket_read: recv: count: %d",count);
		if (count < 0) {
			DLOG(LOG_DEBUG|LOG_SYSERR,"socket_read: recv");
			return -1;
		} else if (count < 1) {
			if (errno == EAGAIN)
				continue;
			else
				break;
		}
#if DEBUG
		else
			if (debug > dlevel+1) bindump("socket_read", buffer, count);
#endif
		bytes_left -= count;
		ptr += count;
		total += count;
	}
	DLOG(LOG_DEBUG2,"socket_read: total bytes read: %d",total);
	sp->in += total;
	return(total);
}

/* Recv any available data from a socket */
int socket_recv(SOCK sp,void *buffer,unsigned short bufsize) {
	register int bytes;

	/* If timeout set, use it */
	DLOG(LOG_DEBUG2,"socket_recv: timeout: %d", sp->timeout);
	if (sp->timeout) {
		if (socket_ready(sp, sp->timeout) == 0) {
			DLOG(LOG_DEBUG2,"socket_recv: timeout waiting for ready.");
			errno = ENOENT;
			return -2;
		}
	}

	DLOG(LOG_DEBUG2,"socket_recv: receiving from socket %d...",
			sp->s);
	do {
		bytes = recv(sp->s,buffer,bufsize,0);
		DLOG(LOG_DEBUG3,"socket_recv: bytes: %d",bytes);
		if (bytes < 0) {
			DLOG(LOG_DEBUG|LOG_SYSERR,"socket_recv: recv");
			return -1;
		}
	} while(bytes < 1 && errno == EAGAIN);

	DLOG(LOG_DEBUG2,"socket_recv: bytes: %d",bytes);
	sp->in += bytes;
	return bytes;
}

/* Write to a socket */
int socket_write(SOCK sp,void *buffer,unsigned short bufsize) {
	register char *ptr = (char *) buffer;
	register int bytes_left = bufsize,count;
	int total;

	DLOG(LOG_DEBUG2, "socket_write: writing %d bytes to socket %d...", bufsize,sp->s);
	total = 0;
	while(bytes_left) {
#if DEBUG
		bindump("socket_write",buffer, count);
#endif
		count = send(sp->s,ptr,bytes_left,0);
		DLOG(LOG_DEBUG3,"socket_write: send: count: %d",count);
		if (count < 0) {
			DLOG(LOG_DEBUG|LOG_SYSERR,"socket_write: send");
			return -1;
		} else if (count < 1) {
			if (errno == EAGAIN)
				continue;
			else
				break;
		}
		bytes_left -= count;
		ptr += count;
		total += count;
	}
	DLOG(LOG_DEBUG2,"socket_write: total bytes written: %d",total);
	sp->out += total;
	return(total);
}

unsigned short socket_htons(unsigned short val) {
	return htons(val);
}

unsigned short socket_ntohs(unsigned short val) {
	return ntohs(val);
}

unsigned long socket_htonl(unsigned long val) {
	return htonl(val);
}


unsigned long socket_ntohl(unsigned long val) {
	return ntohl(val);
}
