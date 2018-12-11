#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "oprecomp.h"

#include "pca_.h"
// #include "dat.h"
extern float datiInput2[channels][window];

void PCA_mrrr(int samples, int variables, float *input, int components, float *output);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s PROBLEM_SIZE NUM_THREADS\n", argv[0]);
        return 1;
    }

    int K = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    omp_set_dynamic(0); // enforce number of threads
    omp_set_num_threads(num_threads);

    printf("# problem_size: %d\n", K);
    printf("# channels: %d\n", channels);
    printf("# window: %d\n", window);
    printf("# num_threads: %d\n", num_threads);

    float output2[K][window];

    oprecomp_start();
    do {
        PCA_mrrr(window, channels, (float *)datiInput2, K, (float *)output2);
    } while (oprecomp_iterate());
    oprecomp_stop();

    /* const int not_used = -1;
    float output[K][window];
    PCA(not_used, not_used, output);

    float err = 0;
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < window; j++) {
            float tmp = fabsf(output[i][j]) - fabsf(output2[i][j]);
            if (tmp > err || isnan(tmp)) err = tmp;
        }
    }
    printf("# maxerr: %e\n", err); */
}
