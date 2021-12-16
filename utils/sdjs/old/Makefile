
PROGNAME=$(shell basename $(shell pwd))
SRCS=js.c editline.c
CFLAGS+=-DHAVE_STDLIB -DSYS_UNIX -DHAVE_TCGETATTR -DUSE_DIRENT

ifeq ($(TARGET),win32)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32
else ifeq ($(TARGET),win64)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32 -D_WIN64
else
SRCS+=sysunix.c
CFLAGS+=-DXP_UNIX -DSVR4 -DSYSV -D_BSD_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy  -DDEBUG -DDEBUG_root
endif

JS=yes

include ../../Makefile.sd

#LIBS+=-lnspr4 -lm
