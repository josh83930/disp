# Top level makefile, the real shit is at src/Makefile

default: all

.DEFAULT:
	cd src && $(MAKE) $@

VERSION=1.2.1

archive:
	git archive --format=tar --prefix=disp-$(VERSION)/ v$(VERSION) | gzip > disp-$(VERSION).tar.gz

install:
	@mkdir -p $(DESTDIR)/bin $(DESTDIR)/lib
	install -m 0755 bin/* $(DESTDIR)/bin
	install -m 0755 lib/* $(DESTDIR)/lib
	cd python && $(MAKE) $@ PREFIX=$(DESTDIR)

.PHONY: install archive
