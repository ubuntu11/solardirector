
SMANET=yes

PROGNAME=si
SRCS=main.c can.c smanet.c info.c si.c siconfig.c jsfuncs.c
SCRIPTS=*.js

include ../../Makefile.sd

t: t.c $(DEPS)
	$(FULLCC) -o t t.c si.c can.c ../../utils/siutil/display.c $(LIBS)
