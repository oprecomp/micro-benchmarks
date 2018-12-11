# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

CFLAGS ?= -O3 -g -Wall -Wextra -std=c99
CFLAGS += -I$(PULP_SDK_HOME)/install/ws/include
LDFLAGS ?= -L$(PULP_SDK_HOME)/install/ws/lib -loprecomp -lcxl -lgvlauncher -lpthread

all:: build/conv

build/conv: host.c
	[ -d build ] || mkdir build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
