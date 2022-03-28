
LIBNAME=$(shell basename $(shell pwd))
SRCS=common.c debug.c opts.c log.c utils.c battery.c inverter.c getpath.c memory.c daemon.c driver.c homedir.c list.c types.c conv.c json.c parson.c cfg.c mqtt.c uuid.c message.c findconf.c buffer.c tmpdir.c exec.c agent.c client.c fork.c config.c notify.c dns.c influx.c stredit.c
SCRIPTS=battery.js charger.js core.js init.js inverter.js utils.js

CLEANFILES=js_solard_inverter.h

include ../Makefile.sd

inverter.c: js_solard_inverter.h

js_solard_inverter.h: $(TOOLS)/mkjs.sh
	$(TOOLS)/mkjs.sh inverter.h solard_inverter 'inv->' 1 ro
