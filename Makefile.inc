
ifeq ($(TARGET),)
	OS=$(shell uname)
	TARGET=$(shell echo $(OS) | tr "[:upper:]" "[:lower:]")
endif

ID=$(shell id -u)

ifeq ($(ID),0)
PREFIX?=/usr/local
else
PREFIX?=~
#PREFIX?=/usr/local
endif
ETCDIR?=$(PREFIX)/etc
BINDIR?=$(PREFIX)/bin
SBINDIR?=$(PREFIX)/sbin
LIBDIR?=$(PREFIX)/lib
INCDIR?=$(PREFIX)/include
STATIC?=no

INSTALL=install
ifeq ($(TARGET),pi3)
	ARCH=pi
	OS=Linux
	BITS=32
	CVERS=arm-linux-gnueabihf
	CBINDIR=/usr/$(CVERS)/bin
	CC=$(CBINDIR)/$(CVERS)-gcc
	AR=$(CBINDIR)/$(CVERS)-ar
	RANLIB=$(CBINDIR)/$(CVERS)-ranlib
	STRIP=$(CBINDIR)/$(CVERS)-strip
#	CFLAGS+=-DNULLDEV=\"/dev/null\" -D__USE_FILE_OFFSET64
	DEST_BASE=/usr/$(CVERS)
	DEST_PREFIX=$(DEST_BASE)/$(CVERS)/sysroot
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
else ifeq ($(TARGET),win64)
	ARCH=win64
	OS=Windows
	BITS=64
	EXE=.exe
	CBINDIR=/bin
	CVERS=x86_64-w64-mingw32
	CC=$(CBINDIR)/$(CVERS)-gcc
	AR=$(CBINDIR)/$(CVERS)-ar
	RANLIB=$(CBINDIR)/$(CVERS)-ranlib
	STRIP=$(CBINDIR)/$(CVERS)-strip
	CFLAGS+=-DWINDOWS -DWIN32 -DWIN64
	DEST_BASE=/usr/$(CVERS)
	DEST_PREFIX=$(DEST_BASE)/sys-root/mingw
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
	DYNAMIC=no
else ifeq ($(TARGET),win32)
	ARCH=win32
	OS=Windows
	BITS=32
  	EXE=.exe
	CBINDIR=/bin
	CVERS=i686-w64-mingw32
	CC=$(CBINDIR)/$(CVERS)-gcc
	AR=$(CBINDIR)/$(CVERS)-ar
	RANLIB=$(CBINDIR)/$(CVERS)-ranlib
	STRIP=$(CBINDIR)/$(CVERS)-gcc
	STRIP=$(BASE)/bin/$(VERS)-strip
	CFLAGS+=-DWINDOWS -DWIN32 -D_FILE_OFFSET_BITS=64
	DEST_BASE=/usr/$(CVERS)
	DEST_PREFIX=$(DEST_BASE)/sys-root/mingw
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
	DYNAMIC=no
else ifeq ($(TARGET),solaris)
	MACH := $(shell uname -p)
	ARCH=solaris_$(MACH)
	CC=$(shell if test -x /opt/csw/gcc4/bin/gcc; then echo /opt/csw/gcc4/bin/gcc; else echo gcc; fi)
	INSTALL=ginstall
else
	ARCH=linux
	OS=Linux
	CC=gcc
	AR=ar
	RANLIB=ranlib
	STRIP=strip
#	CFLAGS+=-DNULLDEV=\"/dev/null\" -D__USE_FILE_OFFSET64
#	CFLAGS+=-DXP_UNIX
	DEST_BASE=/usr/local
	DEST_PREFIX=$(DEST_BASE)
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
	TARGET=linux
endif

ifeq ($(BITS),)
	MACH := $(shell uname -m)
	ifeq ($(MACH),x86_64)
		BITS=64
	else
		ifeq ($(MACH),i686)
			BITS=32
		else ifeq ($(MACH),i386)
			BITS=32
		endif
	endif
	ifneq "$(findstring armv7l,$(MACH))" ""
		BITS=32
	endif
endif

# SRC_ROOT is wherever we are at
current_makefile := $(lastword $(MAKEFILE_LIST))
SRC_ROOT ?= $(shell echo $$(dirname $(current_makefile)))

# Local system overrides
ifneq ("$(wildcard /usr/local/lib/make.opts)","")
include /usr/local/lib/make.opts
endif

# Deps
DEPDIR := .deps
CLEANFILES+=.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

FULLCC = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS)

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(FULLCC) -c $<

%.so : %.c
%.so : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(FULLCC) -fPIC -c $< -o $@

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)

# Debugging?
ifeq ($(DEBUG),yes)
	CFLAGS+=-Wall -DDEBUG -D_DEBUG -g -O0
#	DEBUG_SUFFIX=_debug
else
	CFLAGS+=-O2 -pipe
endif

ifeq ($(DEBUG_MEM),yes)
	CFLAGS+=-DDEBUG_MEM
	ifeq ($(MEMWATCH),yes)
		CFLAGS+=-DMEMWATCH
	endif
endif

# Suffixes
#SUFFIX=_$(ARCH)$(DEBUG_SUFFIX)
#SUFFIX=_$(TARGET)$(DEBUG_SUFFIX)
SUFFIX=$(DEBUG_SUFFIX)

CFLAGS+=-I.

ifeq ($(shell test -d $(INCDIR) && echo yes),yes)
	CFLAGS+=-I$(INCDIR)
endif

# Uses LIBOS?
ifeq ($(LIBOS),yes)
	CFLAGS+=-I$(SRC_ROOT)/libos
	LIBS+=-L$(SRC_ROOT)/libos -los$(SUFFIX)
	DEPS+=$(SRC_ROOT)/libos/libos$(SUFFIX)$(LIBEXT)
endif

# Uses VIM?
ifeq ($(VIM),yes)
	UTIL=yes
	SOAP=yes
	CFLAGS+=-I$(SRC_ROOT)/vim
	LIBS+=-L$(SRC_ROOT)/vim -lvim$(SUFFIX)
	DEPS+=$(SRC_ROOT)/vim/libvim$(SUFFIX)$(LIBEXT)
endif

# Uses libutil?
ifeq ($(UTIL),yes)
  ifneq ($(wildcard $(SRC_ROOT)/util),)
	CFLAGS+=-I$(SRC_ROOT)/util
	LIBS+=-L$(SRC_ROOT)/util -lutil$(SUFFIX)
	DEPS+=$(SRC_ROOT)/util/libutil$(SUFFIX)$(LIBEXT)
  else
	LIBS+=-lutil$(SUFFIX)
	DEPS+=$(LIBDIR)/libutil$(SUFFIX)$(LIBEXT)
  endif
endif

# Uses SOAP?
ifeq ($(SOAP),yes)
	SOAP_CFLAGS=-DWITH_OPENSSL -DWITH_DOM -DWITH_COOKIES -DWITH_GZIP
	CFLAGS+=-I$(SRC_ROOT)/soap
	LIBS+=-L$(SRC_ROOT)/soap -lsoap$(SUFFIX)
	DEPS+=$(SRC_ROOT)/soap/libsoap$(SUFFIX)$(LIBEXT)
	LIBS+=-lgsoapssl -lz -lssl -lcrypto -ldl
endif

# Uses ODBC?
ifeq ($(ODBC),yes)
	ifneq ($(wildcard $(SRC_ROOT)/db),)
		CFLAGS+=-I$(SRC_ROOT)/db
		LIBS+=-L$(SRC_ROOT)/db -ldb$(SUFFIX)
		DEPS+=$(SRC_ROOT)/db/libdb$(SUFFIX)$(LIBEXT)
	else
		LIBS+=-ldb$(SUFFIX)
		DEPS+=$(LIBDIR)/libdb$(SUFFIX)$(LIBEXT)
	endif
	ifeq ($(OS),Windows)
		LIBS+=-lodbc32
	else
		LIBS+=-lodbc
	endif
endif

# Uses MySQL?
ifeq ($(MYSQL),yes)
	LIBS+=-lmysqlclient
endif

# Uses libxml?
ifeq ($(XML),yes)
	CFLAGS+=$(shell xml2-config --cflags)
	LIBS+=$(shell xml2-config --libs)
endif

ifeq ($(AV),yes)
	LIBS+=-lavformat -lavcodec -lavutil -lfaac -lfaad -lx264 -lxvidcore -lmp3lame -lvorbis -lvorbisenc -logg -lz -lbz2 -lx264 -l$(PTHREADS) $(AVLIBADD)
endif

ifeq ($(STATIC),yes)
	_DOSTATIC=yes
else
	_DOSTATIC=no
endif

ifeq ($(CURL),yes)
	LIBS+=-lcurl
	ifeq ($(_DOSTATIC),yes)
		_DOSTATIC=no
#		LIBS+=-lnghttp2 -lidn2 -lrtmp -lssh2 -lpsl -lssl -lcrypto -lssl -lcrypto
#		LIBS+=-lffi -lpcre -lpthread -lmount -ldl -lblkid -lresolv -lz -lselinux
#		CFLAGS+=$(shell pkg-config --cflags --static libcurl)
#		LIBS+=$(shell pkg-config --libs --static libcurl)
	endif
endif

# Uses pthreads? (needed @ end)
ifeq ($(PTHREADS),yes)
	ifeq ($(OS),Windows)
#		LIBS+=-lwinpthread
		LIBS+=-lpthread
	else
		LIBS+=-lpthread
	endif
endif

ifeq ($(OS),Windows)
	LIBS+=-lws2_32
endif


#####################

#
# End of mods
#

OBJS+=$(SRCS:%.c=%.o)
#OBJS?=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))

.PHONY: _all

##############################
#
##################### LIB
#
##############################

ifneq ($(LIBNAME),)
ALIB=lib$(LIBNAME)$(SUFFIX).a
CLEANFILES+=$(ALIB)
ifeq ($(SHARED),yes)
ifeq ($(OS),Windows)
	SHEXT=.dll
	SHLDADD+=-Wl,--export-all-symbols -Wl,--enable-auto-import
else
	SHEXT=.so
endif
SLIB=lib$(LIBNAME)$(SUFFIX)$(SHEXT)
SOBJS+=$(SRCS:%.c=%.so)
CLEANFILES+=$(SLIB) $(SOBJS)
_all: $(ALIB) $(SLIB)
else
_all: $(ALIB)
endif

$(ALIB): $(OBJS)
	$(AR) cr $(ALIB) $(OBJS)
	$(RANLIB) $(ALIB)

ifeq ($(SHARED),yes)
$(SLIB): $(SOBJS)
#	$(CC) $(LDADD) $(SHLDADD) -shared -o $(SLIB) $(SOBJS) $(SHAREDLIBS)
	$(FULLCC) $(LDADD) $(SHLDADD) -shared -o $(SLIB) $(SOBJS) $(SHAREDLIBS)
endif

ifneq ("$(wildcard $(SCRIPTS))","")
install:
	mkdir -p $(LIBDIR)/$(LIBNAME)
	$(INSTALL) -m 755 $(INSTALL_USER) $(SCRIPTS) $(LIBDIR)/$(LIBNAME)
endif

else # not LIBNAME
##############################
#
##################### PROG
#
##############################

ifneq ($(PROGNAME),)
var=value
# Static binary?
ifeq ($(_DOSTATIC),yes)
	LDADD+=-static
else
	# Dynamic binary?
	ifeq ($(DYNAMIC),yes)
		LDADD+=-rdynamic
		LIBS+=-ldl
	endif
endif
PROG=$(PROGNAME)$(EXE)
_all: $(PROG)
$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDADD) -o $(PROG) $(OBJS) $(LIBS) $(ENDLIBS)
#	$(FULLCC) $(LDADD) -o $(PROG) $(OBJS) $(LIBS) $(ENDLIBS)

debug: $(PROG)
	gdb ./$(PROG)

SERVICE=$(PROGNAME).service
PROGCONF=$(PROGNAME).conf

ifneq ("$(wildcard $(SERVICE))","")
install: $(PROG) /etc/systemd/system/$(SERVICE)
else
install: $(PROG)
endif
ifneq ("$(wildcard $(PROGCONF))","")
	mkdir -p $(ETCDIR)
	$(INSTALL) -m 644 $(INSTALL_USER) $(PROGCONF) $(ETCDIR)/$(PROGCONF)
endif
ifneq ("$(wildcard $(SCRIPTS))","")
	mkdir -p $(LIBDIR)/agents/$(PROGNAME)
	$(INSTALL) -m 755 $(INSTALL_USER) $(SCRIPTS) $(LIBDIR)/agents/$(PROGNAME)
endif
ifneq ("$(wildcard $(FILES))","")
	mkdir -p $(LIBDIR)
	$(INSTALL) -m 755 $(INSTALL_USER) $(FILES) $(LIBDIR)
endif
	mkdir -p $(BINDIR)
	$(INSTALL) -m 755 $(INSTALL_USER) $(PROG) $(BINDIR)/$(PROG)
endif

ifneq ("$(wildcard $(SERVICE))","")
/etc/systemd/system/$(SERVICE): $(SERVICE)
ifeq ($(ID),0)
	$(INSTALL) -m 644 $(INSTALL_USER) $< $@
	systemctl daemon-reload
	systemctl enable $(SERVICE)
endif
release: $(PROG)
	mkdir -p $(SRC_ROOT)/releases/$(TARGET)
	$(MAKE) PREFIX=$(SRC_ROOT)/releases/$(TARGET) TARGET=$(TARGET) install

else # not PROGNAME
all:

endif # PROGNAME

endif # LIBNAME

clean:
	rm -rf $(LIB) $(PROG) $(OBJS) *.o $(CLEANFILES) __sd_build.h

$(DEPFILES):
include $(wildcard $(DEPFILES))

#__sd_build.h:
#	echo "#define __SD_BUILD $$(date '+%Y%m%d%H%M')" > __sd_build.h

#__sd_build:
#	echo "#define __SD_BUILD $$(date '+%Y%m%d%H%M')" > __sd_build.h

main.o: $(SRCS)
	@mkdir -p .deps
	@echo "#define __SD_BUILD $$(date '+%Y%m%d%H%M')" > __sd_build.h
	$(FULLCC) -c main.c
