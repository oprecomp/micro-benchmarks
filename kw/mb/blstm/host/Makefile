# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

CFLAGS ?= -O3 -g -Wall -Wextra -std=c99
CPPFLAGS ?= -O3 -g -Wall -Wextra -std=c++11 -fopenmp
CFLAGS += -I$(PULP_SDK_HOME)/install/ws/include
CPPFLAGS += -I$(PULP_SDK_HOME)/install/ws/include -I./include
LDFLAGS ?= -L$(PULP_SDK_HOME)/install/ws/lib -loprecomp -lcxl -lpulpvplauncher -lpthread

all: clean build/neuron build/blstm

clean:
	rm -f build/neuron.o build/blstm

build/neuron: ./src/neuron.cpp
	[ -d build ] || mkdir build
	g++ $(CPPFLAGS) -o build/neuron.o -c ./src/neuron.cpp $(LDFLAGS)

build/blstm: src/host.cpp
	[ -d build ] || mkdir build
	g++ $(CPPFLAGS) -o $@ $< build/neuron.o $(LDFLAGS)
