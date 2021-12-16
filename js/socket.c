#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsapi.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
//#include "js_util_functions.h"
#include "socket.h"

static SocketInformation *Socket_createInfo();
static JSBool Socket_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static void Socket_dtor(JSContext *cx, JSObject *obj);
static JSBool Socket_connect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool Socket_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool Socket_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool Socket_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool Socket_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *ret);
//static JSBool Socket_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *ret);
static char *errnomsg(int e);


//static JSObject *SocketPrototype = NULL;
static JSClass js_Socket_class = {
    "Socket",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    Socket_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec Socket_methods[] = {
	{"connect", Socket_connect, 0, 0, 0},
	{"read", Socket_read, 0, 0, 0},
	{"write", Socket_write, 0, 0, 0},
	{"close", Socket_close, 0, 0, 0},
    {NULL}
};

#define JSSOCK_SOL_TCP 1
#define JSSOCK_PF_INET 2
#define JSSOCK_SOCK_STREAM 3

static JSPropertySpec Socket_static_properties[] = {
    {"SOL_TCP", JSSOCK_SOL_TCP, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT, Socket_getProperty, NULL},
    {"PF_INET", JSSOCK_PF_INET, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT, Socket_getProperty, NULL},
    {"SOCK_STREAM", JSSOCK_SOCK_STREAM, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT, Socket_getProperty, NULL},
    {NULL}
};


static char *errnomsg(int e){
    char *msg = NULL;
    switch (e){
        case EAFNOSUPPORT:
            msg = strdup("Address family not supported on this system.");
            break;
		case EINVAL:
			msg = strdup("Invalid protocol or family.");
			break;
        case EACCES:
            msg = strdup("Permission denied.");
            break;
        case EMFILE:
            msg = strdup("No descriptors available for process, too many open files and sockets.");
            break;
        case ENFILE:
            msg = strdup("No descriptors available system wide, too many open files and sockets.");
            break;
        case ENOMEM:
        case ENOBUFS:
            msg = strdup("No memory available for socket.");
            break;
        case EPROTONOSUPPORT:
            msg = strdup("Specified protocol is not supported on this system.");
            break;
		case EBADF:
			msg = strdup("Bad socket descriptor.");
			break;
		case ECONNREFUSED:
			msg = strdup("Connection refused.");
			break;
		case EISCONN:
			msg = strdup("Socket is already connected.");
			break;
        default:
            msg = strdup("Unknown error occured.");
    }
    return msg;
}


JSObject *js_InitSocketClass(JSContext *cx, JSObject *globalObj) {
//	JSObject *globalObj = JS_GetGlobalObject(cx);
	JSObject *obj;

    /* Define the file object. */
    obj = JS_InitClass(cx, globalObj, globalObj, &js_Socket_class, Socket_ctor, 1, NULL, Socket_methods, Socket_static_properties, NULL);
	return obj;
#if 0
    if (!SocketPrototype){  
        return JS_FALSE;
    }
	return JS_TRUE;
#endif
}

#if 0
JSObject *Socket_getPrototypeObject(){
	return SocketPrototype;
}
#endif


static SocketInformation *Socket_createInfo(){
	SocketInformation *info;
	info = malloc(sizeof(*info));
	if (info == NULL){
		return NULL;
	}
	else {
		info->isConnected = 0;
		info->addr = NULL;
		info->socketDescriptor = INVALID_SOCKET;
		return info;
	}
}

static JSBool Socket_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    SocketInformation *info=NULL;
    if (argc < 1){
        /*If no parameters, we assume the most common type of socket which is: */
        /*socket(PF_INET, SOCK_STREAM, SOL_TCP) */
        info = Socket_createInfo();
        if (info == NULL){
            char *msg = errnomsg(errno);
            jsval exception = printf_exception(cx, "Cannot create socket descriptor: %s", msg);
            free(msg);
            JS_SetPendingException(cx, exception);
            return JS_FALSE;
        }

        info->socketDescriptor=socket(PF_INET, SOCK_STREAM, SOL_TCP);
        if (info->socketDescriptor == INVALID_SOCKET){
            char *msg = errnomsg(errno);
            jsval exception = printf_exception(cx, "Cannot create socket descriptor: %s", msg);
            free(msg);
            JS_SetPendingException(cx, exception);
            return JS_FALSE;
        }

        JS_SetPrivate(cx, obj, info);
        return JS_TRUE;
    }
    else if (argc == 3){
        int domain, type, proto;

        info = Socket_createInfo();
        if (info == NULL){
            char *msg = errnomsg(errno);
            jsval exception = printf_exception(cx, "Cannot create socket descriptor: %s", msg);
            free(msg);
            JS_SetPendingException(cx, exception);
            return JS_FALSE;
        }

        domain=JSVAL_TO_INT(argv[0]);
        type = JSVAL_TO_INT(argv[1]);
        proto =JSVAL_TO_INT(argv[2]);

        info->socketDescriptor=socket(domain, type, proto);
        if (info->socketDescriptor == INVALID_SOCKET){
            char *msg = errnomsg(errno);
            jsval exception = printf_exception(cx, "Cannot create socket descriptor: %s", msg);
            free(msg);
            JS_SetPendingException(cx, exception);
            return JS_FALSE;
        }
        JS_SetPrivate(cx, obj, info);

        return JS_TRUE;
    }
    else {
        jsval exception = printf_exception(cx, "Incorrect parameters for Socket()");
        JS_SetPendingException(cx, exception);
        return JS_FALSE;
    }
}

static void Socket_dtor(JSContext *cx, JSObject *obj){
    SocketInformation *info = JS_GetPrivate(cx, obj);
    if (info != NULL){
		if (info->addr != NULL){
			free(info->addr);
		}
		if (info->socketDescriptor != INVALID_SOCKET){
			close(info->socketDescriptor);
	        info->socketDescriptor=INVALID_SOCKET;
		}
		free(info);
    }
}

static JSBool Socket_connect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	SocketInformation *info = JS_GetPrivate(cx, obj);
	if (argc == 1){
		/*They passed an object specifying how to connect.*/
		JSObject *obj;
		if (JS_ValueToObject(cx, argv[0], &obj) == JS_TRUE){
			int family;
			JSObject *details;
			jsval propVal;


			JS_GetProperty(cx, obj, "details", &propVal);
			JS_ValueToObject(cx, propVal, &details);

			JS_GetProperty(cx, obj, "family", &propVal);
            if (propVal == JSVAL_VOID){
					jsval exception = printf_exception(cx, "family property is required.");
					JS_SetPendingException(cx, exception);
					return JS_FALSE;
            }
			family = JSVAL_TO_INT(propVal);

			if (family == JSSOCK_PF_INET){
				char *address;
				int port;
				struct hostent *hostent;

				JS_GetProperty(cx, details, "address", &propVal);
				address = JS_GetStringBytes(JS_ValueToString(cx, propVal));

				JS_GetProperty(cx, details, "port", &propVal);
				port = JSVAL_TO_INT(propVal);

				
				/*We have our connection details.  Now we need to resolve address if required */
				hostent = gethostbyname(address);
				if (hostent == NULL){
					jsval exception = printf_exception(cx, "Cannot resolve host name '%s'", address);
					JS_SetPendingException(cx, exception);
					return JS_FALSE;
				}
				else {
					struct in_addr *addr = (struct in_addr*)hostent->h_addr_list[0];
					struct sockaddr_in *conAddr = malloc(sizeof(*conAddr));

					conAddr->sin_family = PF_INET;
					conAddr->sin_port = htons(port);
					conAddr->sin_addr = *addr;

					info->addr = (struct sockaddr*)conAddr;

					if (connect(info->socketDescriptor, info->addr, sizeof(*(info->addr))) == 0){
						info->isConnected = 1;
						return JS_TRUE;
					}
					else {
						char *msg = errnomsg(errno);
						jsval exception = printf_exception(cx, "Cannot connect to '%s:%d': %s", address, port, msg);
						free(msg);
						JS_SetPendingException(cx, exception);
						return JS_FALSE;
					}
				}
			}
            else {
                jsval exception = printf_exception(cx, "Protocol family unsupported.");
                JS_SetPendingException(cx, exception);
                return JS_FALSE;
            }
		}
		else {
			jsval exception = printf_exception(cx, "Expected object as first parameter");
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
	else {
		jsval exception = printf_exception(cx, "Unknown parameter");
		JS_SetPendingException(cx, exception);
		return JS_FALSE;
	}
}

static JSBool Socket_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	SocketInformation *info = JS_GetPrivate(cx, obj);
	if (info != NULL){
		if (info->socketDescriptor != INVALID_SOCKET && info->isConnected == 1){
			JSString *socketData;
			char *buff = JS_malloc(cx, 1024);
			int bytesRead = 0;
			memset(buff, 0, 1024);

			bytesRead = read(info->socketDescriptor, buff, 1024);
			
			socketData = JS_NewString(cx, buff, bytesRead);
			*rval = STRING_TO_JSVAL(socketData);
			return JS_TRUE;
		}
		else {
			jsval exception = printf_exception(cx, "Not Connected.");
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
	else {
		jsval exception = printf_exception(cx, "No socket.");
		JS_SetPendingException(cx, exception);
		return JS_FALSE;
	}
}

static JSBool Socket_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	SocketInformation *info = JS_GetPrivate(cx, obj);
	if (info != NULL){
		if (info->socketDescriptor != INVALID_SOCKET && info->isConnected == 1){
			if (argc == 1){
				JSString *dataToWrite;
				char *data;

				dataToWrite = JS_ValueToString(cx, argv[0]);
				data = JS_GetStringBytes(dataToWrite);

				write(info->socketDescriptor, data, strlen(data));
				return JS_TRUE;
			}
			else {
				jsval exception = printf_exception(cx, "Invalid parameters");
				JS_SetPendingException(cx, exception);
				return JS_FALSE;
			}
		}
		else {
			jsval exception = printf_exception(cx, "Not Connected.");
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
	else {
		jsval exception = printf_exception(cx, "No socket.");
		JS_SetPendingException(cx, exception);
		return JS_FALSE;
	}
}

static JSBool Socket_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	SocketInformation *info = JS_GetPrivate(cx, obj);
	if (info != NULL){
		if (info->socketDescriptor != INVALID_SOCKET){
			if (info->isConnected == 1){
				close(info->socketDescriptor);
			}
		}
	}
	return JS_TRUE;
}

static JSBool Socket_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *ret){
    int idNum = JSVAL_TO_INT(id);
    switch(idNum){
		case JSSOCK_PF_INET:
			*ret = INT_TO_JSVAL(PF_INET);
			break;
		case JSSOCK_SOCK_STREAM:
			*ret = INT_TO_JSVAL(SOCK_STREAM);
			break;
		case JSSOCK_SOL_TCP:
			*ret = INT_TO_JSVAL(SOL_TCP);
			break;
        default:
            *ret = INT_TO_JSVAL(idNum);
    }
    return JS_TRUE;
}
