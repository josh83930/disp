# install python bindings with `make install`

PREFIX?=/usr/local

default: build

.DEFAULT:
	./setup.py $@

build:
	./setup.py build

install:
	./setup.py install --prefix=$(PREFIX)

.PHONY: install build
