
SMANET=yes

PROGNAME=$(shell basename $(shell pwd))
SRCS=main.c

#all: 
#	echo $(SRCS)
#	echo $(OBJS)

include ../../Makefile.sd

ifneq ($(OS),Windows)
LIBS+=-lreadline
endif

