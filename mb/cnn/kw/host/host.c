// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include <stdio.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <liboprecomp.h>


struct wed {
	int64_t N, M, KI, KO, U, V;
	float *in_x; // KI*N*M
	float *in_w; // KO*KI*U*V
	volatile float *out_y; // KO*N*M
};


inline uint32_t lfsr_next(uint32_t *lfsr) {
	int fb = *lfsr & 1;
	*lfsr >>= 1;
	if (fb) *lfsr ^= 0xB4BCD35C;
	return *lfsr;
}


inline float lfsr_next_float(uint32_t *lfsr) {
	uint32_t v = lfsr_next(lfsr);
	return v * powf(2.0f, -31.0f) - 1.0f;
}


int main(int argc, char **argv) {
	opc_error_t err;

	// Open the PULP binary based on what has been passed on the command line.
	if (argc != 2 && argc != 8) {
		fprintf(stderr, "usage: %s BINARY [N M KI KO U V]\n", argv[0]);
		return 1;
	}

	// Load the kernel.
	opc_kernel_t knl = opc_kernel_new();
	err = opc_kernel_load_file(knl, argv[1]);
	if (err != OPC_OK) {
		opc_perror("opc_kernel_load_file", err);
		return 1;
	}

	// Prepare a buffer of random input data and space for the output.
	struct wed wed = {
		.N = 32,
		.M = 32,
		.KI = 16,
		.KO = 16,
		.U = 3,
		.V = 3,
	};
	if (argc == 8) {
		wed.N  = (int64_t)atoi(argv[2]);
		wed.M  = (int64_t)atoi(argv[3]);
		wed.KI = (int64_t)atoi(argv[4]);
		wed.KO = (int64_t)atoi(argv[5]);
		wed.U  = (int64_t)atoi(argv[6]);
		wed.V  = (int64_t)atoi(argv[7]);
	}
	wed.in_x = calloc(wed.KI*wed.N*wed.M, sizeof(float));
	wed.in_w = calloc(wed.KO*wed.KI*wed.U*wed.V, sizeof(float));
	wed.out_y = calloc(wed.KO*wed.N*wed.M, sizeof(float));

	uint32_t lfsr = 1;
	for (int64_t ki = 0; ki < wed.KI; ++ki) {
		for (int64_t n = 0; n < wed.N; ++n) {
			for (int64_t m = 0; m < wed.M; ++m) {
				wed.in_x[ki*wed.N*wed.M + n*wed.M + m] = lfsr_next_float(&lfsr);
			}
		}
	}
	for (int64_t ko = 0; ko < wed.KO; ++ko) {
		for (int64_t ki = 0; ki < wed.KI; ++ki) {
			for (int64_t u = 0; u < wed.U; ++u) {
				for (int64_t v = 0; v < wed.V; ++v) {
					wed.in_w[ko*wed.KI*wed.U*wed.V + ki*wed.U*wed.V + u*wed.V + v] = lfsr_next_float(&lfsr);
				}
			}
		}
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
	size_t num_wrong = 0, num_checked = 0;
	for (int64_t ko = 0; ko < wed.KO; ++ko) {
		for (int64_t n = 0; n < wed.N; ++n) {
			for (int64_t m = 0; m < wed.M; ++m) {
				float exp = 0.0;
				for (int64_t ki = 0; ki < wed.KI; ++ki) {
					for (int64_t u = 0; u < wed.U; ++u) {
						int64_t nu = n - wed.U/2 + u;
						if (nu < 0 || nu >= wed.N) continue;
						for (int64_t v = 0; v < wed.V; ++v) {
							int64_t mv = m - wed.V/2 + v;
							if (mv < 0 || mv >= wed.M) continue;
							exp += wed.in_x[ki*wed.N*wed.M + nu*wed.M + mv] * wed.in_w[ko*wed.KI*wed.U*wed.V + ki*wed.U*wed.V + u*wed.V + v];
						}
					}
				}

				float act = wed.out_y[ko*wed.N*wed.M + n*wed.M + m];
				float diff = act - exp;
				diff *= diff;
				if (diff > 1e-6) {
					++num_wrong;
					fprintf(stderr, "MISMATCH: output %ldx%ldx%ld, expected = %g, actual = %g\n", ko, n, m, exp, act);
				}
				++num_checked;
			}
		}
	}
	if (num_wrong == 0)
		fprintf(stderr, "all %lu outputs correct\n", num_checked);
	else
		fprintf(stderr, "mismatch in %lu/%lu outputs\n", num_wrong, num_checked);

	// Clean up.
	opc_dev_free(dev);
	opc_kernel_free(knl);
	return num_wrong > 0;
}
