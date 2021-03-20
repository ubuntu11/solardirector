
LIBNAME=solard
SRCS=init.c agent.c battery.c inverter.c opts.c cfg.c conv.c list.c module.c mqtt.c debug.c parson.c utils.c uuid.c worker.c json.c daemon.c memory.c client.c message.c log.c devserver.c

include ../Makefile.sd

t: t.c
	$(CC) -o t t.c $(LIB)
