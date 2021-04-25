
ifeq ($(TARGET),)
	OS=$(shell uname)
	TARGET=$(shell echo $(OS) | tr "[:upper:]" "[:lower:]")
endif

ifeq ($(whoami),root)
PREFIX?=/usr/local
else
#PREFIX?=~
PREFIX?=/usr/local
endif
ETCDIR?=$(PREFIX)/etc
BINDIR?=$(PREFIX)/bin
SBINDIR?=$(PREFIX)/sbin
LIBDIR?=$(PREFIX)/lib
INCDIR?=$(PREFIX)/include

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
	OS=win
	BITS=64
	EXE=.exe
	CBINDIR=/bin
	CVERS=x86_64-w64-mingw32
	CC=$(CBINDIR)/$(CVERS)-gcc
	AR=$(CBINDIR)/$(CVERS)-ar
	RANLIB=$(CBINDIR)/$(CVERS)-ranlib
	STRIP=$(CBINDIR)/$(CVERS)-strip
#	CFLAGS+=-DWIN32 -DWIN64 -DNULLDEV=\"NUL\"
	DEST_BASE=/usr/$(CVERS)
	DEST_PREFIX=$(DEST_BASE)/sys-root/mingw
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
	DYNAMIC=no
else ifeq ($(TARGET),win32)
	ARCH=win32
	OS=win
	BITS=32
  	EXE=.exe
	CBINDIR=/bin
	CVERS=i686-w64-mingw32
	CC=$(CBINDIR)/$(CVERS)-gcc
	AR=$(CBINDIR)/$(CVERS)-ar
	RANLIB=$(CBINDIR)/$(CVERS)-ranlib
	STRIP=$(CBINDIR)/$(CVERS)-gcc
	CFLAGS+=-DWIN32 -DWIN64 -DNULLDEV=\"NUL\"
	STRIP=$(BASE)/bin/$(VERS)-strip
#	CFLAGS+=-DWIN32 -DNULLDEV=\"NUL\" -D_FILE_OFFSET_BITS=64
	DEST_BASE=/usr/$(CVERS)
	DEST_PREFIX=$(DEST_BASE)/sys-root/mingw
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
	DYNAMIC=no
#$(findstring find,in)
else ifneq (,$(findstring linux,$(TARGET)))
	ARCH=linux
	OS=Linux
	CC=gcc
	AR=ar
	RANLIB=ranlib
	STRIP=strip
	INSTALL=install
#	CFLAGS+=-DNULLDEV=\"/dev/null\" -D__USE_FILE_OFFSET64
	DEST_BASE=/usr/local
	DEST_PREFIX=$(DEST_BASE)
	DEST_BINDIR=$(DEST_PREFIX)/bin
	DEST_INCDIR=$(DEST_IPREFIX)/include
	DEST_LIBDIR=$(DEST_IPREFIX)/lib
else ifeq ($(TARGET),solaris)
	MACH := $(shell uname -p)
	ARCH=solaris_$(MACH)
	CC=$(shell if test -x /opt/csw/gcc4/bin/gcc; then echo /opt/csw/gcc4/bin/gcc; else echo gcc; fi)
	INSTALL=ginstall
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

# Debugging?
ifeq ($(DEBUG),yes)
CFLAGS+=-Wall -DDEBUG -D_DEBUG -g
DEBUG_SUFFIX=_debug
else
CFLAGS+=-O2 -pipe
endif

ifeq ($(DEBUG_MEM),yes)
CFLAGS+=-DDEBUG_MEM
endif

ifeq ($(SHARED),yes)
ifeq ($(OS),win)
LIBEXT=.dll.os
LDADD=-Wl,--export-all-symbols -Wl,--enable-auto-import
else
LIBEXT=.so
ifneq ($LIBNAME),)
CFLAGS+=-fPIC
endif
endif
else
LIBEXT=.a
endif

# Static binary?
ifeq ($(STATIC),yes)
LDFLAGS+=-static
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
  LIBS+=-lgsoapssl -lz -lssl -lcrypto
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
ifeq ($(OS),win)
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

ifeq ($(LIBJS),yes)
CFLAGS+=-I../libjs
DEPS+=../libjs/libjs_$(OS)$(LIBEXT)
LIBS+=-L../libjs -ljs_$(OS)
endif

ifeq ($(AV),yes)
LIBS+=-lavformat -lavcodec -lavutil -lfaac -lfaad -lx264 -lxvidcore -lmp3lame -lvorbis -lvorbisenc -logg -lz -lbz2 -lx264 -l$(PTHREADS) $(AVLIBADD)
endif

ifeq ($(DYNAMIC),yes)
LDFLAGS+=-rdynamic
LIBS+=-ldl
endif

# Uses pthreads? (needed @ end)
ifeq ($(PTHREADS),yes)
ifeq ($(OS),win)
#LIBS+=-lpthreadGC2
LIBS+=-lpthread
else
LIBS+=-lpthread
endif
endif

ifeq ($(OS),win)
LIBS+=-lws2_32
endif

#
# End of mods
#

OBJS=$(SRCS:.c=.o)

.PHONY: _all
ifneq ($(LIBNAME),)
LIB=lib$(LIBNAME)$(SUFFIX)$(LIBEXT)
_all: $(LIB)
$(LIB): $(OBJS)
ifeq ($(SHARED),yes)
	echo $(CC)
	$(CC) $(LDADD) -shared -o $(LIB) $(OBJS)
else
	$(AR) cr $(LIB) $(OBJS)
	$(RANLIB) $(LIB)
endif
endif

ifneq ($(PROGNAME),)
PROG=$(PROGNAME)$(EXE)
_all: $(PROG)
$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

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
	sudo mkdir -p $(ETCDIR)
	sudo $(INSTALL) -m 644 -o root -g root $(PROGCONF) $(ETCDIR)/$(PROGCONF)
endif
	sudo mkdir -p $(BINDIR)
	sudo $(INSTALL) -m 755 -o root -g root $(PROG) $(BINDIR)/$(PROG)
endif

ifneq ("$(wildcard $(SERVICE))","")
/etc/systemd/system/$(SERVICE): $(SERVICE)
	sudo $(INSTALL) -m 644 -o root -g root $< $@
	sudo systemctl daemon-reload
	sudo systemctl enable $(SERVICE)
endif

clean:
	rm -rf $(LIB) $(PROG) $(OBJS) *.o $(CLEANFILES)


##.SUFFIXES: .c .o$(SUFFIX)
##.c.o$(SUFFIX):
#.SUFFIXES: .c .o
#.c.o:
#	$(CC) $(CFLAGS) -c $< -o $@

#
# Deps
#

DEPDIR := .deps
CLEANFILES+=.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
