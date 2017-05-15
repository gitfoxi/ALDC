############################################################################
# Makefile for aldc encode/decode library and sample program
############################################################################
CC = gcc
LD = gcc
CFLAGS = -I. -O0 -Wall -Wextra -pedantic -ansi -c -g
LDFLAGS = -O0 -o
# -O3
OS := $(shell uname)

# libraries
LIBS = -L. -laldc -loptlist

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
	OS = Windows
endif

ifeq ($(OS),Windows)
	ifeq ($(OSTYPE), cygwin)
		EXE = .exe
		DEL = rm
	else
		EXE = .exe
		DEL = del
	endif
else	#assume Linux/Unix
	EXE =
	DEL = rm -f
endif

ifeq ($(OS),Darwin)
  MEMSTREAM_O = memstream.o
  FMEMOPEN_O = fmemopen.o
else
  MEMSTREAM_O =
endif

# define the method to be used for searching for matches (choose one)
# brute force
FMOBJ = brute.o

# linked list
# FMOBJ = list.o

# hash table
# FMOBJ = hash.o

# Knuth–Morris–Pratt search
# FMOBJ = kmp.o

# binary tree
# FMOBJ = tree.o

LZOBJS = $(FMOBJ) aldc.o

all:		test sample$(EXE) libaldc.a liboptlist.a

test: libaldc.a test.o liboptlist.a
		$(LD) test.o $< $(LIBS) $(LDFLAGS) $@

test.o:	test.c aldc.h
		$(CC) $(CFLAGS) $<

sample$(EXE):	sample.o libaldc.a liboptlist.a
		$(LD) $< $(LIBS) $(LDFLAGS) $@

sample.o:	sample.c aldc.h optlist.h
		$(CC) $(CFLAGS) $<

libaldc.a:	$(LZOBJS) bitfile.o $(MEMSTREAM_O) $(FMEMOPEN_O)
		ar crv libaldc.a $(LZOBJS) bitfile.o $(MEMSTREAM_O) $(FMEMOPEN_O)
		ranlib libaldc.a

aldc.o:	aldc.c lzlocal.h bitfile.h
		$(CC) $(CFLAGS) $<

brute.o:	brute.c lzlocal.h
		$(CC) $(CFLAGS) $<

list.o:		list.c lzlocal.h
		$(CC) $(CFLAGS) $<

hash.o:		hash.c lzlocal.h
		$(CC) $(CFLAGS) $<

kmp.o:		kmp.c lzlocal.h
		$(CC) $(CFLAGS) $<

tree.o:		tree.c lzlocal.h
		$(CC) $(CFLAGS) $<

bitfile.o:	bitfile.c bitfile.h
		$(CC) $(CFLAGS) $<

liboptlist.a:	optlist.o
		ar crv liboptlist.a optlist.o
		ranlib liboptlist.a

optlist.o:	optlist.c optlist.h
		$(CC) $(CFLAGS) $<

memstream.o: memstream.c memstream.h
		$(CC) $(CFLAGS) $<

clean:
		$(DEL) *.o
		$(DEL) *.a
		$(DEL) sample$(EXE)

.PHONY: printvars

printvars:
				@$(foreach V,$(sort $(.VARIABLES)), $(if $(filter-out environment% default automatic, $(origin $V)),$(warning $V=$($V) ($(value $V)))))
