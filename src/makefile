# Makefile for gmake.
# Go to binary directory and call the Makefile.Project

# $Id: makefile,v 1.7 1995/05/29 19:19:11 ruten Exp $

BASEDIR := $(shell pwd)

%::
	$(MAKE) -C ../bin VPATH=$(BASEDIR) -f $(BASEDIR)/Makefile.Project $@

all:
	$(MAKE) -C ../bin VPATH=$(BASEDIR) -f $(BASEDIR)/Makefile.Project

# create binary directory
bin:
	cd ..; mkdir bin; ln -s bin lib
