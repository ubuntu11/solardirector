
PROG=sbagent

all: sbagent

sbagent: sbagent.py
	cp sbagent.py sbagent

install:
	$(INSTALL) -m 755 $(INSTALL_USER) $(PROG) $(BINDIR)/$(PROG)

include ../../Makefile.sd
