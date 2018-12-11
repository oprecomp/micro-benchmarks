# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

VPATH = $(dir $(firstword $(MAKEFILE_LIST)))
CFLAGS ?= -O3 -fopenmp -Wall -Wextra -Werror-implicit-function-declaration -I$(VPATH)/../../common
LDFLAGS ?= -lm -fopenmp -lrt

build: sample

test: sample
	./sample 26 4

sample: main.c.o oprecomp.c.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.c.o: ../../common/%.c
	$(CC) $(CFLAGS) -c -o $@ $^
