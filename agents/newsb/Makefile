
PROGNAME=sb
SRCS=main.c sb.c info.c config.c sma_strings.c sma_object.c
#SRCS=t.c

CURL=yes

include ../../Makefile.sd

test:: test.c sb.c sb.h sma_object.o sma_strings.o
	$(FULLCC) -o test test.c sma_strings.o sma_object.o $(LIBS)

mkh: mkh.c
	$(FULLCC) -o mkh mkh.c $(LIBS)
