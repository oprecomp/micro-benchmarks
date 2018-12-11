// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include <stdio.h>
#include <stdlib.h>
#include <liboprecomp.h>


struct wed {
	uint64_t num_words;
	int64_t *input;
	volatile int64_t *output;
};


int main(int argc, char **argv) {
	opc_error_t err;

	// Open the PULP binary based on what has been passed on the command line.
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: %s BINARY [NUM_WORDS]\n", argv[0]);
		return 1;
	}

	// Load the kernel.
	opc_kernel_t knl = opc_kernel_new();
	err = opc_kernel_load_file(knl, argv[1]);
	if (err != OPC_OK) {
		opc_perror("opc_kernel_load_file", err);
		return 1;
	}

	// Prepare a buffer of random input words, and a buffer for the outputs. Use
	// a simple LFSR to come up with some random values.
	struct wed wed = { .num_words = 10000 };
	if (argc == 3) {
		wed.num_words = (uint64_t)atoi(argv[2]);
	}
	wed.input  = calloc(wed.num_words, sizeof(int64_t));
	wed.output = calloc(wed.num_words, sizeof(int64_t));

	uint32_t lfsr = 1;
	for (uint64_t i = 0; i < wed.num_words; ++i) {
		int fb = lfsr & 1;
		lfsr >>= 1;
		if (fb) lfsr ^= 0xB4BCD35C;
		wed.input[i] = (int32_t)lfsr;
	}

	// Open a device and offload a job.
	opc_dev_t dev = opc_dev_new();
	err = opc_dev_open_any(dev);
	if (err != OPC_OK) {
		opc_perror("opc_dev_open_any", err);
		return 1;
	}

	err = opc_dev_launch(dev, knl, &wed, NULL);
	if (err != OPC_OK) {
		opc_perror("opc_dev_launch", err);
		return 1;
	}
	opc_dev_wait_all(dev);

	// Check the result.
	size_t num_wrong = 0;
	for (uint64_t i = 0; i < wed.num_words; ++i) {
		int64_t exp = wed.input[i] * wed.input[i];
		if (wed.output[i] != exp) {
			++num_wrong;
			fprintf(stderr, "MISMATCH: word %6lu, expected = %016lx, actual = %016lx\n", i, exp, wed.output[i]);
		}
	}
	if (num_wrong == 0)
		fprintf(stderr, "all correct\n");

	// Clean up.
	opc_dev_free(dev);
	opc_kernel_free(knl);
	return num_wrong > 0;
}
