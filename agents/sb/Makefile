
PROGNAME=sb
SRCS=main.c sb.c info.c config.c utils.c strings.c objects.c jsfuncs.c

CURL=yes

include ../../Makefile.sd

test:: test.c $(filter-out main.c,$(SRCS)) $(DEPS)
	$(FULLCC) -o test test.c $(filter-out main.o,$(OBJS)) $(LIBS)

t: t.o $(SRCS)
	$(FULLCC) -o t t.c sb.o utils.o objects.o info.o config.o jsfuncs.o $(LIBS)

mkh: mkh.c
	$(FULLCC) -o mkh mkh.c $(LIBS)

mkob: mkob.c sma_objects.c sma_strings.c $(DEPS)
	$(FULLCC) -o $@ $< $(LIBS)
