
#TARGET=win32
LIBNAME=solard
SRCS=init.c agent.c battery.c inverter.c opts.c cfg.c conv.c list.c module.c mqtt.c debug.c parson.c utils.c uuid.c worker.c json.c daemon.c memory.c client.c message.c log.c devserver.c findconf.c getpid.c buffer.c socket.c

include ../Makefile.sd

t: t.c
	$(CC) -o t t.c $(LIB)
