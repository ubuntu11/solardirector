
PROGNAME=sb
SRCS=main.c sb.c info.c config.c
#SRCS=t.c

CURL=yes

include ../../Makefile.sd

test:: test.c sb.c
	$(FULLCC) -o test test.c $(LIBS) && ./test

mkh: mkh.c
	$(FULLCC) -o mkh mkh.c $(LIBS)
