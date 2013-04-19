# Makefile for cppp

CC = gcc -ggdb
CFLAGS = -Wall -W

OBJLIST = unixisms.o error.o strset.o clex.o exptree.o ppproc.o main.o

cppp: $(OBJLIST)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LOADLIBES)

unixisms.o : unixisms.c unixisms.h
error.o    : error.c error.h
strset.o   : strset.c strset.h main.h
clex.o     : clex.c clex.h main.h error.h
exptree.o  : exptree.c exptree.h main.h error.h strset.h clex.h
ppproc.o   : ppproc.c ppproc.h main.h error.h strset.h clex.h exptree.h
main.o     : main.c main.h unixisms.h error.h strset.h ppproc.h

clean:
	rm -f $(OBJLIST) cppp

dist:
	tar -czvf cppp.tar.gz Makefile clex.[ch] cppp.1 error.[ch] \
             exptree.[ch] main.[ch] ppproc.[ch] strset.[ch] unixisms.[ch]