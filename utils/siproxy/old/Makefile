
PROG=$(shell basename $(shell pwd))
MYBMM_SRC=../mybmm
MODULES=$(shell cat $(MYBMM_SRC)/Makefile | grep ^MODULES | awk -F= '{ print $$2 }')
SRCS=main.c utils.c cfg.c conv.c list.c $(MODULES)
OBJS=$(SRCS:.c=.o)
CFLAGS=-I$(MYBMM_SRC)
CFLAGS+=-DCHKUM
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG
LIBS+=-ldl -lgattlib -lglib-2.0 -lpthread
LDFLAGS+=-rdynamic


vpath %.c $(MYBMM_SRC)

all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile *.h

install: $(PROG)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/sbin

clean:
	rm -f $(PROG) $(OBJS)
