
SMANET=yes

LIBNAME=smanet
SRCS=init.c frame.c packet.c command.c channel.c value.c

all: $(LIB) try

CLEANFILES+=try mktab

include ../Makefile.sd

MYTP=../transports/serial.c ../transports/rdev.c
try: try.c $(LIB) ../lib/libsolard_debug.a $(MYTP)
	$(CC) $(CFLAGS) -o $@ try.c $(MYTP) $(LIBS)

debug: try
	gdb ./try
