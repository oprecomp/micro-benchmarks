# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

CFLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-parameter
CFLAGS += -fPIC -std=c99 -I../libcxl

all: build/liboprecomp.so build/liboprecomp.a build/elfparse build/staticpack

build/liboprecomp.a: build/liboprecomp.c.o
	@echo "  [AR] $@"
	@[ -d build ] || mkdir build
	@$(AR) rcs $@ $^

build/liboprecomp.so: build/liboprecomp.c.o
	@echo "  [LD] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -shared -o $@ $^

build/liboprecomp.c.o: liboprecomp.c liboprecomp.h
	@echo "  [CC] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -c -o $@ liboprecomp.c

build/elfparse: elfparse.c build/liboprecomp.a
	@echo "  [CC] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -o $@ $^

build/staticpack: staticpack.c build/liboprecomp.a
	@echo "  [CC] $@"
	@[ -d build ] || mkdir build
	@$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf build

install:
	install -D build/liboprecomp.a $(PULP_SDK_HOME)/install/ws/lib
	install -D build/liboprecomp.so $(PULP_SDK_HOME)/install/ws/lib
	install -D liboprecomp.h $(PULP_SDK_HOME)/install/ws/include
	mkdir -p $(PULP_SDK_HOME)/install/cfg
	plpinfo tree --config-options="plt_loader/load-binary_eval=os.environ.get('PULP_SDK_HOME') + '/install/bin/rom-oprecompkw'" --config-options="soc/cluster/pe0/fetch_enable=true" --config-options="chip/boot_from_rom=true" --config-options="loader/boot/mode=rom" > $(PULP_SDK_HOME)/install/cfg/oprecompkw_config.json
