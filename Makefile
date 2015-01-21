# Top level makefile, the real shit is at src/Makefile

default: all

.DEFAULT:
	cd src && $(MAKE) $@

install:
	@mkdir -p $(DESTDIR)/bin $(DESTDIR)/lib
	install -m 0755 bin/* $(DESTDIR)/bin
	install -m 0755 lib/* $(DESTDIR)/lib

.PHONY: install
