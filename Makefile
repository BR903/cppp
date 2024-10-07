#  Makefile for cppp

# Where to install
#
prefix = /usr/local
version = 2.6

CC = gcc
CFLAGS = -Wall -Wextra -O2

OBJLIST = gen.o unixisms.o error.o symset.o clexer.o exptree.o ppproc.o cppp.o

cppp: $(OBJLIST)

gen.o     : gen.c gen.h
unixisms.o: unixisms.c unixisms.h
error.o   : error.c error.h gen.h
symset.o  : symset.c symset.h gen.h
clexer.o  : clexer.c clexer.h gen.h error.h
exptree.o : exptree.c exptree.h gen.h error.h symset.h clexer.h
ppproc.o  : ppproc.c ppproc.h gen.h error.h symset.h clexer.h exptree.h
cppp.o    : cppp.c gen.h unixisms.h error.h symset.h ppproc.h

install:
	install -D ./cppp $(prefix)/bin/cppp
	install -D ./cppp.1 $(prefix)/share/man/man1/cppp.1

check:
	./pace

clean:
	rm -f $(OBJLIST) cppp

dist:
	rm -f cppp-$(version).tar cppp-$(version).tar.gz
	ln -s . cppp-$(version)
	sed "s,^,cppp-$(version)/," MANIFEST | tar -cf cppp-$(version).tar -T -
	gzip -9 cppp-$(version).tar
	rm cppp-$(version)
