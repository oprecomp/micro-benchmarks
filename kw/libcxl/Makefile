# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

CFLAGS ?= -O3 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -fPIC -I$(PULP_SDK_HOME)/install/ws/include

all: build/libcxl.so build/libcxl.a

build/libcxl.a: build/libcxl.c.o
	@echo "  [AR] $@"
	@[ -d build ] || mkdir build
	@$(AR) rcs $@ $^

build/libcxl.so: build/libcxl.c.o
	@echo "  [CC] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -shared -o $@ $^

build/libcxl.c.o: libcxl.c libcxl.h
	@echo "  [CC] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -c -o $@ libcxl.c

clean:
	rm -rf build

install:
	install -D build/libcxl.a $(PULP_SDK_HOME)/install/ws/lib
	install -D build/libcxl.so $(PULP_SDK_HOME)/install/ws/lib
	install -D libcxl.h $(PULP_SDK_HOME)/install/ws/include