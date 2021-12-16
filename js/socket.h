#ifndef JSSOCKET_H
#define JSSOCKET_H


JSBool register_class_Socket(JSContext *cx);
JSObject *Socket_getPrototypeObject();

#ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
#endif 

struct _SocketInformation {
	int socketDescriptor;
	int isConnected;
	struct sockaddr *addr;
};

typedef struct _SocketInformation SocketInformation;

#endif
