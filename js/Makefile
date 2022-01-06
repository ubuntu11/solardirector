
LIBNAME=$(shell basename $(shell pwd))
SRCS=jsapi.c jsarena.c jsarray.c jsatom.c jsbool.c jscntxt.c jsdate.c jsdbgapi.c jsdhash.c jsdtoa.c jsemit.c jsexn.c jsfile.c jsfun.c jsgc.c jshash.c jsinterp.c jsinvoke.c jsiter.c jslock.c jslog2.c jslong.c jsmath.c jsnum.c jsobj.c jsopcode.c jsparse.c jsprf.c jsregexp.c jsscan.c jsscope.c jsscript.c jsstr.c jsutil.c jsxdrapi.c jsxml.c prmjtime.c jsdtracef.c json.c jsglobal.c jsengine.c jsconv.c jsprintf.c console.c socket.c jscan.c jsinflux.c
ifeq ($(TARGET),win32)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32
else ifeq ($(TARGET),win64)
CFLAGS+=-DXP_WIN -D_WINDOWS -D_WIN32 -D_WIN64
else
CFLAGS+=-DXP_UNIX -D_DEFAULT_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy
endif
#CFLAGS+=-DJS_THREADSAFE

CLEANFILES+=jskwgen jsautokw.h test

ifneq ($(TARGET),)
CFLAGS+=-DCROSS_COMPILE
endif

include ../Makefile.sd

#jsapi.o: jsautocfg.h

#jsautocfg.h: jscpucfg
#	./$< > $@

#jscpucfg: jscpucfg.c
#	$(CC) $(CFLAGS) -o $@ $<

jsscan.o: jsautokw.h

jsautokw.h: jskwgen
	./$< $@

jskwgen: jskwgen.c
	gcc $(CFLAGS) -o $@ $<


test: test.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) -lm

js: js.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) -lm
