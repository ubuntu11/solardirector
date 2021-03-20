
PROG=$(shell basename $(shell pwd))
MYBMM_SRC=../mybmm
SRCS=main.c preh.c can.c utils.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-I$(MYBMM_SRC) -DPREH_DUMP
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG
LIBS+=-lpthread
LDFLAGS+=-rdynamic

vpath %.c $(MYBMM_SRC)

all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

include $(MYBMM_SRC)/Makefile.dep

debug: $(PROG)
	gdb $(PROG)

$(OBJS): Makefile

install: $(PROG)
	sudo install -m 755 -o bin -g bin $(PROG) /usr/sbin

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull
