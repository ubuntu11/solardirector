
SMANET=yes

LIBNAME=$(shell basename $(shell pwd))
SRCS=init.c frame.c packet.c command.c channel.c value.c

all: $(LIB) try

CLEANFILES+=try mktab

include ../Makefile.sd

try: try.c $(LIB) $(MYTP)
	$(CC) $(CFLAGS) -o $@ try.c $(MYTP) $(LIBS)

debug: try
	gdb ./try
