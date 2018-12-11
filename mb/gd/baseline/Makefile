# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

CXXFLAGS ?= -Wall -Wextra -std=c++11 -O3
LDFLAGS ?= -flto -O3 -lpthread

# Make sure this Makefile can be executed in an arbitrary build directory. This
# essentially allows out-of-source builds. Setting the VPATH to the directory
# containing the Makefile will allow make to find dependencies in that
# directory.
VPATH = $(dir $(firstword $(MAKEFILE_LIST)))

build:
# build: mnist-train

test: tf-learn
# test: mnist-train
# 	./mnist-train --data data/prepared/mb/gd --epochs 1

tf-learn: tf-learn.py amester-tool
ifdef OPRECOMP_HAS_TENSORFLOW
	python3 $<
else
	echo "skipping TensorFlow model"
endif

amester-tool: ../../common/amester-tool.c ../../common/oprecomp.c
	$(CC) -O3 -Wall -o $@ $^ -lrt

mnist-train: train.cpp.o
	$(CXX) $^ $(LDFLAGS) -o $@

mnist-test: test.cpp.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
