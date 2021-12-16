
SMANET=yes

PROGNAME=si
SRCS=main.c smanet.c si.c charge.c read.c write.c config.c info.c jsfuncs.c

include ../../Makefile.sd

CLEANFILES=js_si_info.h

config.c: si_info_props.c

#jsfuncs.c: js_si_info.h
#js_si_info.h: $(TOOLS)/mkjs.sh
#	${TOOLS}/mkjs.sh si.h si_info 's->info.' end ro

si_info_props.c: $(TOOLS)/mkcp.sh
	$(TOOLS)/mkcp.sh  si.h si_info 's->info.' "CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE" > si_info_props.c
