
NAME=smanet
SRCS=busevents.c byteorder.c chanvalrepo.c CMakeCCompilerId.c defractionizer.c driver_layer.c feature_tests.c fractionizer.c getini.c iorequest.c ttyip.c ip_generic.c libyasdi.c mastercmd.c mempool.c minmap.c minqueue.c netchannel.c netdevice.c netpacket.c objman.c os_linux.c plant.c prot_layer.c repository.c router.c scheduler.c serial_posix.c smadata_layer.c smanet.c stateconfig.c statedetection.c stateident.c statereadchan.c statewritechan.c statistic_writer.c sunnynet.c timer.c tools.c ysecurity.c libyasdimaster.c main.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-Wall -g -DDEBUG
LIB=lib$(NAME).a

all: $(LIB) siutil

include Makefile.dep

$(LIB): $(OBJS)
	ar cr $(LIB) $(OBJS)

siutil: siutil.c $(LIB)
	cc $(CFLAGS) -o siutil siutil.c -L. -l$(NAME) -lpthread

debug: siutil
	gdb ./siutil

clean:
	rm -f $(LIB) $(OBJS)
