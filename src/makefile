# Makefile for gmake.
# call the Makefile.Project

# $Id: makefile,v 1.7 1995/05/29 19:19:11 ruten Exp $

BASEDIR := $(shell pwd)

%::
	$(MAKE) VPATH=$(BASEDIR) -f $(BASEDIR)/Makefile.Project $@

all:
	$(MAKE) VPATH=$(BASEDIR) -f $(BASEDIR)/Makefile.Project

