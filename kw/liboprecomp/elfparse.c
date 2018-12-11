// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
//
// A simple test that parses PULP ELF32 binaries.

#include "liboprecomp.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv) {
	opc_error_t err;

	// Open the PULP binary based on what has been passed on the command line.
	if (argc != 2) {
		fprintf(stderr, "usage: %s BINARY\n", argv[0]);
		return 1;
	}

	// Load the kernel.
	opc_kernel_t knl = opc_kernel_new();
	err = opc_kernel_load_file(knl, argv[1]);
	if (err != OPC_OK) {
		opc_perror("opc_kernel_load_file", err);
		return 1;
	}

	// Dump some information about the kernel.
	opc_kernel_dump_info(knl, stdout);

	// Clean up.
	opc_kernel_free(knl);
	return 0;
}


void cxl_afu_attach() {}
void cxl_afu_open_dev() {}
void cxl_afu_open_h() {}
void cxl_afu_next() {}
