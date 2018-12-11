// Copyright (c) 2017 ORPECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include "oprecomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>


int main(int argc, char **argv) {

	// Parse the parameters that determine the benchmark's execution.
	if (argc != 3) {
		fprintf(stderr, "usage: %s PROBLEM_SIZE NUM_THREADS\n", argv[0]);
		return 1;
	}
	int problem_size = 1 << atoi(argv[1]);
	int num_threads = atoi(argv[2]);

	printf("# problem_size: %d\n", problem_size);
	printf("# num_threads: %d\n", num_threads);

	// Initialize the problem. Don't measure this.
	double *data = calloc(problem_size, sizeof(double));
	int i;
	for (i = 0; i < problem_size; ++i) {
		data[i] = (double)(rand() % (1 << 16)) / (1 << 16);
	}

	// Set the number of threads we want for solving the problem.
	omp_set_dynamic(0); // enforce number of threads
	omp_set_num_threads(num_threads);

	// Start measuring performance.
	fprintf(stderr, "oprecomp_start\n");
	oprecomp_start();

	do {
		#pragma omp parallel for
		for (i = 0; i < problem_size; ++i) {
			double f = data[i];
			f = sqrt(f) * 2;
			f = f*f / 2 + 1.0;
			data[i] = f;
		}
	} while (oprecomp_iterate());

	// Stop measuring performance.
	oprecomp_stop();
	fprintf(stderr, "oprecomp_stop\n");

	// Pretend that we have some other data to be logged. All lines starting
	// with a "# <header>: <value>" will be concatenated into a result by the
	// benchmark wrapper script.
	printf("# a: %d\n", rand());
	printf("# b: %d\n", rand());
	printf("# c: %f\n", data[0]);

	// Clean up.
	free(data);

	return 0;
}
