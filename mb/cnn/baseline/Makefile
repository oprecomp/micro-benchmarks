# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

BMDIR = $(dir $(firstword $(MAKEFILE_LIST)))
CFLAGS += -DCCN_X86 -DCCN_TILING -std=c99 -fopenmp

# Make sure this Makefile can be executed in an arbitrary build directory. This
# essentially allows out-of-source builds. Setting the VPATH to the directory
# containing the Makefile will allow make to find dependencies in that
# directory.
VPATH = $(dir $(firstword $(MAKEFILE_LIST)))

build:
# build: cnn.a

test: tf-infer

tf-infer: tf-infer.py amester-tool
ifdef OPRECOMP_HAS_TENSORFLOW
	python3 $<
else
	echo "skipping TensorFlow model"
endif

amester-tool: ../../common/amester-tool.c ../../common/oprecomp.c
	$(CC) -O3 -Wall -o $@ $^ -lrt

cnn.a: \
	ConvLayer.c.o \
	ConvPoolLayer.c.o \
	DenseLayer.c.o \
	PoolLayer.c.o \
	huffman.c.o \
	linalg.c.o
	$(AR) -r $@ $^

%.c.o: $(BMDIR)/src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@
