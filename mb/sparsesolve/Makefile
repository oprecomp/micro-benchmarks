# this Makefile is intended to be run from a 'build' subdirectory
VPATH = $(dir $(firstword $(MAKEFILE_LIST)))

CC = gcc
CFLAGS = -std=gnu99 -Wall -O3 -I$(VPATH)../common
LDFLAGS = -lm -lrt

.SECONDARY:

.PHONY: build

build: sparsesolve

sparsesolve: sparsesolve.o mmio.o matrix.o cg.o ir.o oprecomp.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	/bin/rm -rf tags core *.o sparsesolve test.tmp *.eps *.pdf

tags: *.c *.h
	ctags *.c *.h

test: sparsesolve
	./sparsesolve data/prepared/mb/sparsesolve/bcsstk01.mtx 1 1e-7 10000 1e-7 100

%.o: %.c cg.h vector.h matrix.h
	$(CC) $(CFLAGS) -c $< -o $@

oprecomp.o: $(VPATH)../common/oprecomp.c
	$(CC) $(CFLAGS) -c $<
