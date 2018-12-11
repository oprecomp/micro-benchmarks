# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
#
# Builds the PULP bootloader without the run time. We need a separate link
# script since the code is loacted at a different static location (boot ROM).

PULP_APP = boot-oprecompkw
PULP_APP_FC_SRCS = boot.c
PULP_APP_FC_ASM_SRCS = crt0.S
PULP_CFLAGS = -O3 -g -fno-jump-tables
PULP_LDFLAGS = -Tlink.ld -nostdlib
CONFIG_OPT = options/rt/no-crt0=true options/rt/no-link-script=true options/rt/no-rt=true

include $(PULP_SDK_HOME)/install/rules/pulp_rt.mk
