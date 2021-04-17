
SMANET=yes

LIBNAME=smanet
SRCS=init.c frame.c packet.c command.c channel.c value.c
TRANSPORTS=serial,rdev

all: $(LIB) try

CLEANFILES+=try mktab

include ../Makefile.sd

try: try.c $(LIB) ../lib/libsolard_debug.a
#	cc $(CFLAGS) -o $@ try.c ../transports/serial.c ../transports/rdev.c -L. -lsmanet_debug -L../lib -lsolard_debug -ldl
	cc $(CFLAGS) -o $@ try.c $(LIBS)

debug: try
	gdb ./try
