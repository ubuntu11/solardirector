
SMANET=yes

LIBNAME=$(shell basename $(shell pwd))
SRCS=smanet.c frame.c packet.c command.c channel.c value.c jsfuncs.c

CLEANFILES+=try mktab

include ../Makefile.sd

try: try.c $(LIB) $(MYTP)
	$(CC) $(CFLAGS) -o $@ try.c $(MYTP) $(LIBS)

debug: try
	gdb ./try
