
PROGNAME=$(shell basename $(shell pwd))
SRCS=main.c si.c can.c smanet.c info.c display.c

vpath %.c ../../agents/si
CFLAGS+=-I../../agents/si

SMANET=yes

include ../../Makefile.sd
