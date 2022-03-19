
LIBNAME=$(shell basename $(shell pwd))
SRCS=jsapi.c jsarena.c jsarray.c jsatom.c jsbool.c jscntxt.c jsdate.c jsdbgapi.c jsdhash.c jsdtoa.c jsemit.c jsexn.c jsfile.c jsfun.c jsgc.c jshash.c jsinterp.c jsinvoke.c jsiter.c jslock.c jslog2.c jslong.c jsmath.c jsnum.c jsobj.c jsopcode.c jsparse.c jsprf.c jsregexp.c jsscan.c jsscope.c jsscript.c jsstr.c jsutil.c jsxdrapi.c jsxml.c prmjtime.c jsdtracef.c jsjson.c jsglobal.c jsengine.c jsconv.c jsprintf.c console.c socket.c jscan.c jsinflux.c jsclass.c

include ../Makefile.sd

ifeq ($(TARGET),win32)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32
else ifeq ($(TARGET),win64)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32 -D_WIN64
else
CFLAGS+=-DXP_UNIX
#CFLAGS+=-DHAVE_VA_COPY -DVA_COPY=va_copy
#CFLAGS+=-D_DEFAULT_SOURCE
#CFLAGS+=-DHAVE_LOCALTIME_R
endif
#CFLAGS+=-DJS_THREADSAFE
CFLAGS+=-DJS_ARENAMETER -DJS_GCMETER -DJS_OPMETER

CLEANFILES+=jskwgen jsautokw.h test

ifneq ($(TARGET),)
CFLAGS+=-DCROSS_COMPILE
endif

jsscan.o: jsautokw.h

jsautokw.h: jskwgen
	./$< $@

jskwgen: jskwgen.c
	gcc $(CFLAGS) -o $@ $<


test: test.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) -lm
