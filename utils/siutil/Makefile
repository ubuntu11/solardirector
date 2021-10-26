
PROGNAME=$(shell basename $(shell pwd))
SRCS=main.c display.c si_info.c si.c read.c write.c charge.c

vpath %.c ../../agents/si
CFLAGS+=-I../../agents/si

SMANET=yes

include ../../Makefile.sd
