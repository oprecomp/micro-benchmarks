// Copyright (c) 2017 OPRECOMP Project
// PULP template by Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
// BLSTM by Dionysios Diamantopoulos <did@zurich.ibm.com>

#include "rt/rt_api.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>


/* BLSTM specific header files */
#include "../include/neuron.h"

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

	/*
	// Snippet for finding the maximum alloc memory on PULP
	// quick n'dirty way to find how much rt_alloc bytes PULP can crunch
	int b = 0;
	//float * array = (float*)rt_alloc(RT_ALLOC_CL_DATA, 10 * HIGHT_IN_PIX * sizeof (float));
	//rt_free(RT_ALLOC_CL_DATA, array, 10 * HIGHT_IN_PIX * sizeof (float));
  while ( rt_alloc(RT_ALLOC_CL_DATA,(1*sizeof(uint64_t))) != NULL) ++b;
  printf("Allocated %d bytes total\n", b*sizeof(uint64_t));
  exit(-1);
	*/

	// Create performacne counter struct and initiate counters
	//rt_perf_t perf;
  //rt_perf_init (&perf);
	//rt_perf_start	(&perf);

	assert (MAX_NUMBER_COLUMNS_TEST_SET - BYPASS_COLUMNS > 0);

	printf("Executing loaded PULP binary on [%d, %d]!\n", rt_cluster_id(), rt_core_id());

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
	uint64_t *buffer = rt_alloc(RT_ALLOC_CL_DATA, chunk_size * sizeof(uint64_t));
	if(!buffer) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }


	union {
			uint32_t t;
			float f;
			char b[sizeof(float)];
	} u;

	#ifdef RECONSTURCT_BW_IMAGE_IN_PULP
	unsigned int numberOfColumns = wed.num_words / HIGHT_IN_PIX;
	#else // RECONSTURCT_BW_IMAGE_IN_PULP
	unsigned int numberOfColumns = wed.num_words / HIGHT_IN_PIX / 2;
	#endif // RECONSTURCT_BW_IMAGE_IN_PULP

	unsigned int numberOfColumnsRemain = numberOfColumns;
	unsigned int numberOfColumnsAlloc = MIN(numberOfColumnsRemain, COLS_PER_KERNEL_EXEC);
	// Prepare local buffers for input fw/bw
	//float image_fw[numberOfColumns * HIGHT_IN_PIX ];
	//float image_bw[numberOfColumns * HIGHT_IN_PIX ];
	size_t vecPredictedStringInd[MAX_NUMBER_COLUMNS_TEST_SET];

	//size_t *vecPredictedStringInd = (size_t *)rt_alloc(RT_ALLOC_CL_DATA, numberOfColumns * sizeof(size_t));
	if(!vecPredictedStringInd) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }

	unsigned int cur_str_len;
	unsigned int vecPredictedStringLen = 0;


	for(unsigned int col = 0; col < numberOfColumns; col += COLS_PER_KERNEL_EXEC) {

		unsigned int curNumberOfColumns = MIN(numberOfColumnsRemain, COLS_PER_KERNEL_EXEC);

		printf("INFO pulp: Running BLSTM for columns %u - %u \n", col, col+curNumberOfColumns-1);

		float * image_fw = (float*)rt_alloc(RT_ALLOC_CL_DATA, curNumberOfColumns * HIGHT_IN_PIX * sizeof (float));
		if(!image_fw) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }
		float * image_bw = (float*)rt_alloc(RT_ALLOC_CL_DATA, curNumberOfColumns * HIGHT_IN_PIX * sizeof (float));
		if(!image_bw) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }

		#ifdef RECONSTURCT_BW_IMAGE_IN_PULP
		size_t cur_num_words = curNumberOfColumns * HIGHT_IN_PIX;
		#else
		size_t cur_num_words = curNumberOfColumns * HIGHT_IN_PIX * 2;
		#endif

		size_t pixels_fed = 0, addr_fw = 0, addr_bw = 0;
		size_t pixels_all = cur_num_words;

		// Process the input in chunks.
		for (size_t i = 0; i < cur_num_words; i += chunk_size) {
			size_t sz = i + chunk_size;
			if (sz > cur_num_words)
				sz = cur_num_words;
			sz -= i;
			printf("INFO pulp: Processing words %d .. %d\n", i, i+sz-1);

			// Copy data from host.
			id = plp_dma_memcpy(
				host2local(wed.input + (i + (col * HIGHT_IN_PIX)) * 8), // remote
				(uint32_t)buffer,            // local
				sz * 8,                      // size
				1                            // remote to local
			);
			plp_dma_wait(id);

			// Fill image buffers
			for (size_t n = 0; n < sz; ++n) {
				u.t = (uint32_t)buffer[n];
				if (pixels_fed < pixels_all) {
					#ifdef RECONSTURCT_BW_IMAGE_IN_PULP
					if (pixels_fed++ < (pixels_all))
					#else // RECONSTURCT_BW_IMAGE_IN_PULP
					if (pixels_fed++ < (pixels_all>>1))
					#endif // RECONSTURCT_BW_IMAGE_IN_PULP
						image_fw[addr_fw++] = u.f;
					else
						image_bw[addr_bw++] = u.f;
					}
			}
		}

		#ifdef RECONSTURCT_BW_IMAGE_IN_PULP
		// Creat an image for backward processing: mirror the columns of the forward image
		for(unsigned int cl = 0; cl < curNumberOfColumns; cl++)	{
			// FIXME: memcpy is not currently working at pulp runtime
			//memcpy(image_bw+cl, image_fw+(numberOfColumns - cl - 1),  HIGHT_IN_PIX * sizeof(float));
			for(unsigned int row = 0; row < HIGHT_IN_PIX; row++)
				image_bw[cl * HIGHT_IN_PIX + row] = image_fw[(curNumberOfColumns - cl - 1) * HIGHT_IN_PIX + row];
		}
		#endif // RECONSTURCT_BW_IMAGE_IN_PULP


		Single_Kernel_BLSTM(
	 		image_fw,
	 		image_bw,
	 		curNumberOfColumns,
	 		vecPredictedStringInd+vecPredictedStringLen,
	 		&cur_str_len);

		numberOfColumnsRemain -= curNumberOfColumns;
  	vecPredictedStringLen += cur_str_len;

		printf("INFO pulp: Found %d ids\nINFO pulp: ids: ", vecPredictedStringLen);
		for (size_t i = 0; i < vecPredictedStringLen ; i++)
			printf("%u ", vecPredictedStringInd[i]);
		printf("\n");

		/* images shall be freed inside neuron.c */
		//if (image_fw != NULL)
		//	rt_free(RT_ALLOC_CL_DATA, image_fw, curNumberOfColumns * HIGHT_IN_PIX * sizeof (float));
		//if (image_bw != NULL)
		//	rt_free(RT_ALLOC_CL_DATA, image_bw, curNumberOfColumns * HIGHT_IN_PIX * sizeof (float));

	} // loop over total columns

	printf("INFO pulp: Found %d ids\nINFO pulp: ids: ", vecPredictedStringLen);
	for (size_t i = 0; i < vecPredictedStringLen ; i++)
		printf("%u ", vecPredictedStringInd[i]);
	printf("\n");

	wed.num_words = vecPredictedStringLen + 1; // 1st address keeps the number of characters found

	unsigned int addr_out = 0;

	for (size_t i = 0; i < wed.num_words; i += chunk_size) {
		size_t sz = i + chunk_size;
		if (sz > wed.num_words)
			sz = wed.num_words;
		sz -= i;
		//printf("Processing output words %d .. %d\n", i, i+sz-1);

		// Fill the local buffer
		for (size_t j = 0; j < chunk_size; j++) {
			if (i+j == 0) // only once for storing length
				buffer[j] = (int64_t)vecPredictedStringLen;
			else
				buffer[j] = (int64_t)vecPredictedStringInd[addr_out++];
		}

		// Copy data back to host.
		id = plp_dma_memcpy(
			host2local(wed.output + i*8), // remote
			(uint32_t)buffer,             // local
			sz * 8,                       // size
			0                             // local to remote
		);
		plp_dma_wait(id);

	}
	//size_t perf_count = rt_perf_get 	(&perf,	0);
  //printf( "Elapsed %u cycles on PULP\n\n", perf_count);
 	//rt_perf_stop( &perf	) ;

	// Clean up.
	rt_free(RT_ALLOC_CL_DATA, buffer, chunk_size * sizeof(uint64_t));
	//rt_free(RT_ALLOC_CL_DATA, vecPredictedStringInd, numberOfColumns * sizeof(size_t));

	return 0;
}
