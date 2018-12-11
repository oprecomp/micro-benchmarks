// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
//
// A simple tool that loads an OPRECOMP kernel binary and densely packs it into
// a binary file. This is useful for later loading in a testbench without PSLSE.

#include "liboprecomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv) {
	opc_error_t err;

	// Open the PULP binary based on what has been passed on the command line.
	if (argc != 3) {
		fprintf(stderr, "usage: %s BINARY OUTPUT\n", argv[0]);
		return 1;
	}

	// Load the kernel.
	opc_kernel_t knl = opc_kernel_new();
	err = opc_kernel_load_file(knl, argv[1]);
	if (err != OPC_OK) {
		opc_perror("opc_kernel_load_file", err);
		return 1;
	}

	// Densely pack the section sources.
	FILE *file = fopen(argv[2], "w");
	if (!file) {
		perror("fopen");
		return 1;
	}
	size_t addr = 0;
	fwrite("OPCTEST", 1, 8, file);
	addr += 8;

	// Insert a placeholder for the WED.
	size_t wed_addr = addr;
	struct {
		uint64_t sec_ptr;
		uint32_t sec_num;
		uint32_t wed_sym;
		uint64_t wed;
		uint64_t result;
	} wed;
	memset(&wed, 0, sizeof(wed));
	wed.wed = 0x0000800000000000;
	fwrite(&wed, sizeof(wed), 1, file);
	addr += sizeof(wed);

	// Write the sections.
	size_t num_sections = opc_kernel_get_num_sections(knl);
	size_t *section_addrs = calloc(num_sections, sizeof(size_t));
	for (size_t i = 0; i < num_sections; ++i) {
		opc_kernel_section_t sec = opc_kernel_get_section(knl, i);
		section_addrs[i] = addr;
		fwrite(sec->src, 1, sec->src_sz, file);
		fprintf(stderr, "section %lu at 0x%lx\n", i, addr);
		addr += sec->src_sz;
	}

	// Write the section table.
	wed.sec_ptr = addr;
	wed.sec_num = num_sections;
	fprintf(stderr, "section table at 0x%lx\n", wed.sec_ptr);
	for (size_t i = 0; i < num_sections; ++i) {
		opc_kernel_section_t sec = opc_kernel_get_section(knl, i);
		struct {
			uint64_t src;
			uint32_t dst;
			uint32_t src_sz;
			uint32_t dst_sz;
		} data = { section_addrs[i], sec->dst, sec->src_sz, sec->dst_sz };
		fwrite(&data, sizeof(data), 1, file);
		addr += sizeof(data);
	}

	// Rewind back to the beginning and write the actual WED.
	wed.wed_sym = opc_kernel_get_wed_symbol(knl);
	fseek(file, wed_addr, SEEK_SET);
	fwrite(&wed, sizeof(wed), 1, file);

	// Clean up.
	fclose(file);
	free(section_addrs);
	opc_kernel_free(knl);
	return 0;
}


void cxl_afu_attach() {}
void cxl_afu_open_dev() {}
void cxl_afu_open_h() {}
void cxl_afu_next() {}
