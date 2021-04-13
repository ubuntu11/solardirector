
LIBNAME=smanet
SRCS=init.c frame.c packet.c command.c channel.c value.c

try: try.c ../transports/serial.c libsmanet_debug.a
	cc $(CFLAGS) -o $@ try.c ../transports/serial.c ../transports/rdev.c -L. -lsmanet_debug -L../lib -lsolard_debug -ldl

include ../Makefile.sd

debug: try
	gdb ./try
