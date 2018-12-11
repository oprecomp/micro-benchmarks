// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include "rt/rt_api.h"
#include <stdio.h>
#include <stdint.h>


// The work element descriptor prepared by the host.
struct wed {
	uint64_t num_words;
	uint64_t input;
	uint64_t output;
};


// Convert a pointer in host memory space to a PULP address that will be routed
// to the host. This needs to be moved into the runtime.
inline uint64_t host2local(uint64_t host) {
	return (host & (((uint64_t)1 << 48)-1)) | ((uint64_t)1 << 48);
}


int main(uint64_t wedptr) {
	int id;

	// Copy the WED from the host machine.
	printf("Loading WED 0x%08lx%08lx\n", (uint32_t)(wedptr >> 32), (uint32_t)wedptr);
	struct wed wed;
	id = plp_dma_memcpy(
		host2local(wedptr), // remote
		(uint32_t)&wed,     // local
		sizeof(wed),        // size
		1                   // remote to local
	);
	plp_dma_wait(id);
	printf("Received WED {\n");
	printf("  num_words = %lu\n", (uint32_t)wed.num_words);
	printf("  input = 0x%08lx%08lx\n", (uint32_t)(wed.input >> 32), (uint32_t)wed.input);
	printf("  output = 0x%08lx%08lx\n", (uint32_t)(wed.output >> 32), (uint32_t)wed.output);
	printf("}\n");

	// Prepare a local buffer.
	const size_t chunk_size = 1000;
	int64_t *buffer = rt_alloc(RT_ALLOC_CL_DATA, chunk_size * sizeof(int64_t));

	// Process the input in chunks.
	for (size_t i = 0; i < wed.num_words; i += chunk_size) {
		size_t sz = i + chunk_size;
		if (sz > wed.num_words)
			sz = wed.num_words;
		sz -= i;
		printf("Processing words %d .. %d\n", i, i+sz-1);

		// Copy data from host.
		id = plp_dma_memcpy(
			host2local(wed.input + i*8), // remote
			(uint32_t)buffer,            // local
			sz * 8,                      // size
			1                            // remote to local
		);
		plp_dma_wait(id);

		// Square all numbers.
		for (size_t n = 0; n < sz; ++n)
			buffer[n] *= buffer[n];

		// Copy data back to host.
		id = plp_dma_memcpy(
			host2local(wed.output + i*8), // remote
			(uint32_t)buffer,             // local
			sz * 8,                       // size
			0                             // local to remote
		);
		plp_dma_wait(id);
	}
	printf("Done\n");

	// Clean up.
	rt_free(RT_ALLOC_CL_DATA, buffer, chunk_size * sizeof(int64_t));
	return 0;
}
