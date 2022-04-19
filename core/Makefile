
include ../make.opts

LIBNAME=$(shell basename $(shell pwd))
SRCS=common.c debug.c opts.c log.c utils.c battery.c inverter.c getpath.c daemon.c driver.c homedir.c list.c types.c conv.c json.c cfg.c mqtt.c uuid.c message.c findconf.c buffer.c tmpdir.c exec.c agent.c client.c fork.c config.c notify.c dns.c influx.c stredit.c websocket.c
SCRIPTS=battery.js charger.js core.js init.js inverter.js utils.js

ifeq ($(DEBUG_MEM),yes)
	ifeq ($(MEMWATCH),yes)
		SRCS+=memwatch.c
	else
		SRCS+=memory.c
	endif
endif

include ../Makefile.sd
