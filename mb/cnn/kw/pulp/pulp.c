// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include "rt/rt_api.h"
#include <stdio.h>
#include <stdint.h>


typedef uint64_t hostptr_t;


// The work element descriptor prepared by the host.
struct wed {
	int64_t N, M, KI, KO, U, V;
	hostptr_t in_x; // KI*N*M
	hostptr_t in_w; // KO*KI*U*V
	volatile hostptr_t out_y; // KO*N*M
};


// Convert a pointer in host memory space to a PULP address that will be routed
// to the host. This needs to be moved into the runtime.
inline uint64_t host2local(hostptr_t host) {
	return (host & (((uint64_t)1 << 48)-1)) | ((uint64_t)1 << 48);
}


#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


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
	printf("  N  = %lu\n", (uint32_t)wed.N);
	printf("  M  = %lu\n", (uint32_t)wed.M);
	printf("  KI = %lu\n", (uint32_t)wed.KI);
	printf("  KO = %lu\n", (uint32_t)wed.KO);
	printf("  U  = %lu\n", (uint32_t)wed.U);
	printf("  V  = %lu\n", (uint32_t)wed.V);
	printf("  in_x  = 0x%08lx%08lx\n", (uint32_t)(wed.in_x  >> 32), (uint32_t)wed.in_x );
	printf("  in_w  = 0x%08lx%08lx\n", (uint32_t)(wed.in_w  >> 32), (uint32_t)wed.in_w );
	printf("  out_y = 0x%08lx%08lx\n", (uint32_t)(wed.out_y >> 32), (uint32_t)wed.out_y);
	printf("}\n");

	// Determine the tiling of the input data via a binary search. This is quite
	// inefficient and should be improved.
	const uint32_t max_size = 65536 - 0x5000;
	const uint32_t max_volume = (max_size - (16 << 10)) / 4;
	printf("Determining Tiling\n");
	printf("  max_volume = %lu\n", max_volume);
	uint32_t TKI, TKO, TN, TM;
	uint32_t f_max = 256, f_min = 0;
	while (f_min < f_max) {
		uint32_t f_mid = (f_max + f_min) / 2;
		printf("  trying %lu..%lu (mid = %lu)\n", f_min, f_max, f_mid);
		TKI = (wed.KI * f_mid + 255) / 256;
		TKO = (wed.KO * f_mid + 255) / 256;
		TN = (wed.N * f_mid + 255) / 256;
		TM = (wed.M * f_mid + 255) / 256;
		uint32_t volume =
			TKI*(TN+wed.U-1)*(TM+wed.V-1) + // input tensor
			TKI*TKO*wed.U*wed.V +           // kernel tensor
			TKO*TN*TM;                      // output tensor
		printf("  TKI=%lu, TKO=%lu, TN=%lu, TM=%lu (volume=%lu)\n", TKI, TKO, TN, TM, volume);
		if (volume < max_volume)
			f_min = f_mid + 1;
		else
			f_max = f_mid - 1;
	}
	TKI = (wed.KI * f_min + 255) / 256;
	TKO = (wed.KO * f_min + 255) / 256;
	TN = (wed.N * f_min  + 255)/ 256;
	TM = (wed.M * f_min  + 255)/ 256;
	printf("Using tiling TKI=%lu, TKO=%lu, TN=%lu, TM=%lu\n", TKI, TKO, TN, TM);

	// Prepare the local buffers that will hold the tiles of the input, kernel,
	// and output.
	const size_t bufsz_x = TKI*(TN+wed.U-1)*(TM+wed.V-1) * sizeof(float);
	const size_t bufsz_w = TKI*TKO*wed.U*wed.V * sizeof(float);
	const size_t bufsz_y = TKO*TN*TM * sizeof(float);
	printf("Allocating %u Bytes for x tile\n", bufsz_x);
	printf("Allocating %u Bytes for w tile\n", bufsz_w);
	printf("Allocating %u Bytes for y tile\n", bufsz_y);
	float *tile_x = rt_alloc(RT_ALLOC_CL_DATA, bufsz_x);
	float *tile_w = rt_alloc(RT_ALLOC_CL_DATA, bufsz_w);
	float *tile_y = rt_alloc(RT_ALLOC_CL_DATA, bufsz_y);
	if (tile_x == NULL || tile_y == NULL || tile_w == NULL)
	{
	  printf("Failed to allocate memory\n");
	  return -1;
	}

	// Perform the outer iterations.
	for (int32_t ko1 = 0; ko1 < wed.KO; ko1 += TKO) {
		const int32_t KO1 = MIN(TKO, wed.KO-ko1);
		for (int32_t n1 = 0; n1 < wed.N; n1 += TN) {
			const int32_t N1 = MIN(TN, wed.N-n1);
			for (int32_t m1 = 0; m1 < wed.M; m1 += TM) {
				const int32_t M1 = MIN(TM, wed.M-m1);
				printf("Tile ko1=%ld..%ld, n1=%ld..%ld, m1=%ld..%ld\n", ko1, ko1+KO1, n1, n1+N1, m1, m1+M1);

				// Clear the output tile.
				plp_dma_barrier();
				memset(tile_y, 1, bufsz_y);

				for (int32_t ki1 = 0; ki1 < wed.KI; ki1 += TKI) {
					const int32_t KI1 = MIN(TKI, wed.KI-ki1);

					// Copy the input and filter tiles.
					const int32_t n_lo = MAX(0, n1 - wed.U/2);
					const int32_t n_hi = MIN(wed.N, n1+N1 + wed.U/2);
					const int32_t m_lo = MAX(0, m1 - wed.V/2);
					const int32_t m_hi = MIN(wed.M, m1+M1 + wed.V/2);

					const int32_t XN1 = n_hi-n_lo;
					const int32_t XM1 = m_hi-m_lo;

					printf("Loading x tile ki1=%lu..%lu, n=%lu..%lu (%ld), m=%lu..%lu (%ld)\n", ki1, ki1+KI1, n_lo, n_hi, XN1, m_lo, m_hi, XM1);
					int merge = 0;
					for (int32_t ki2 = 0; ki2 < KI1; ++ki2) {
						for (int32_t n2 = 0; n2 < XN1; ++n2) {
							plp_dma_memcpy_merge(
								host2local(wed.in_x + (ki1+ki2)*wed.N*wed.M*4 + (n2+n_lo)*wed.M*4 + m_lo*4),
								(uint32_t)tile_x + ki2*XN1*XM1*4 + n2*XM1*4,
								XM1*4,
								PLP_DMA_EXT2LOC,
								merge
							);
							merge = 1;
						}
					}
					plp_dma_barrier();
					merge = 0;
					for (int32_t ko2 = 0; ko2 < KO1; ++ko2) {
						for (int32_t ki2 = 0; ki2 < KI1; ++ki2) {
							plp_dma_memcpy_merge(
								host2local(wed.in_w + (ko1+ko2)*wed.KI*wed.U*wed.V*4 + (ki1+ki2)*wed.U*wed.V*4),
								(uint32_t)tile_w + ko2*TKI*wed.U*wed.V*4 + ki2*wed.U*wed.V*4,
								wed.U*wed.V*4,
								PLP_DMA_EXT2LOC,
								merge
							);
							merge = 1;
						}
					}
					plp_dma_barrier();

					// Perform the calculation.
					for (int32_t ko2 = 0; ko2 < KO1; ++ko2) {
						for (int32_t n2 = 0; n2 < N1; ++n2) {
							for (int32_t m2 = 0; m2 < M1; ++m2) {
								float a = 0.0f;
								for (int32_t ki2 = 0; ki2 < KI1; ++ki2) {
									for (int32_t u = 0; u < wed.U; ++u) {
										const int32_t nu = (n1+n2) - wed.U/2 + u;
										if (nu < n_lo || nu >= n_hi) continue;
										for (int32_t v = 0; v < wed.V; ++v) {
											const int32_t mv = (m1+m2) - wed.V/2 + v;
											if (mv < m_lo || mv >= m_hi) continue;
											const int32_t n_ = nu - n_lo;
											const int32_t m_ = mv - m_lo;
											a += tile_x[ki2*XN1*XM1 + n_*XM1 + m_] * tile_w[ko2*TKI*wed.U*wed.V + ki2*wed.U*wed.V + u*wed.V + v];
										}
									}
								}
								tile_y[ko2*N1*M1 + n2*M1 + m2] += a;
							}
						}
					}
				}

				// Write result tile back.
				int merge = 0;
				for (int32_t ko2 = 0; ko2 < KO1; ++ko2) {
					for (int32_t n2 = 0; n2 < N1; ++n2) {
						plp_dma_memcpy_merge(
							host2local(wed.out_y + (ko1+ko2)*wed.N*wed.M*4 + (n1+n2)*wed.M*4 + m1*4),
							(uint32_t)tile_y + ko2*N1*M1*4 + n2*M1*4,
							M1*4,
							PLP_DMA_LOC2EXT,
							merge
						);
						merge = 1;
					}
				}
			}
		}
	}
	plp_dma_barrier();

	// // Prepare a local buffer.
	// const size_t chunk_size = 1000;
	// int64_t *buffer = rt_alloc(RT_ALLOC_CL_DATA, chunk_size * sizeof(int64_t));

	// // Process the input in chunks.
	// for (size_t i = 0; i < wed.num_words; i += chunk_size) {
	// 	size_t sz = i + chunk_size;
	// 	if (sz > wed.num_words)
	// 		sz = wed.num_words;
	// 	sz -= i;
	// 	printf("Processing words %d .. %d\n", i, i+sz-1);

	// 	// Copy data from host.
	// 	id = plp_dma_memcpy(
	// 		host2local(wed.input + i*8), // remote
	// 		(uint32_t)buffer,            // local
	// 		sz * 8,                      // size
	// 		1                            // remote to local
	// 	);
	// 	plp_dma_wait(id);

	// 	// Square all numbers.
	// 	for (size_t n = 0; n < sz; ++n)
	// 		buffer[n] *= buffer[n];

	// 	// Copy data back to host.
	// 	id = plp_dma_memcpy(
	// 		host2local(wed.output + i*8), // remote
	// 		(uint32_t)buffer,             // local
	// 		sz * 8,                       // size
	// 		0                             // local to remote
	// 	);
	// 	plp_dma_wait(id);
	// }
	// printf("Done\n");

	// Clean up.
	rt_free(RT_ALLOC_CL_DATA, tile_x, bufsz_x);
	rt_free(RT_ALLOC_CL_DATA, tile_w, bufsz_w);
	rt_free(RT_ALLOC_CL_DATA, tile_y, bufsz_y);
	return 0;
}
