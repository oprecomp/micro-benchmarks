#include <stdio.h>
#include <stdlib.h>

// include files for optimized libraries
#if defined USE_ESSL
#include <essl.h>
#elif defined USE_MKL
#include <mkl_cblas.h>
#include <mkl_lapacke.h>
#elif defined USE_LAPACK
#include <cblas.h>
#include <lapacke.h>
#endif

// interface to f2c code

#include "f2c.h"
#include "mrrr.h"

static inline int PCA_ssytd2(char uplo, int n, real *a, int lda, real *d, real *e, real *tau)
{
    int info;
    ssytd2_(&uplo, &n, a, &lda, d, e, tau, &info, 1);
    return info;
}

static inline int PCA_sstemr(char jobz, char range, int n, real *d, real *e, real vl, real vu,
    int il, int iu, int *m, real *w, real *z, int ldz, int nzc, int *isuppz, int *tryrac,
    real *work, int lwork, int *iwork, int liwork)
{
    int info;
    pca_sstemr__(&jobz, &range, &n, d, e, &vl, &vu, &il, &iu, m, w, z, &ldz, &nzc, isuppz,
        tryrac, work, &lwork, iwork, &liwork, &info, 1, 1);
    return info;
}

static inline int PCA_sorm2l(char side, char trans, int m, int n, int k, real *a, int lda, real *tau, real *c, int ldc, real *work)
{
    int info;
    sorm2l_(&side, &trans, &m, &n, &k, a, &lda, tau, c, &ldc, work, &info, 1, 1);
    return info;
}

#ifndef NO_PULP
#include "utils.h"
#include "hwTrace.h"
#endif

#if 1

#define ALLOC(t, v, s) t v[s];
#define FREE(v)

#else

#define ALLOC(t, v, s) t *v = malloc(sizeof(t) * (s));
#define FREE(v) free(v);

#endif

// PCA main routine
// input is a column-major matrix with a row for each sample and a column for each variable
// output is a column-major matrix with a row for each sample and a column for each component

void PCA_mrrr(int samples, int variables, float *input, int components, float *output)
{
    int lwork = 18 * variables;
    int liwork = 10 * variables;
    ALLOC(real, A, variables * variables);
    ALLOC(real, T, samples * variables);
    ALLOC(real, d, variables);
    ALLOC(real, e, variables);
    ALLOC(real, tau, variables);
    ALLOC(int, isuppz, variables * 2);
    ALLOC(real, w, variables);
    ALLOC(real, Z, variables * variables);
    ALLOC(real, work, lwork);
    ALLOC(int, iwork, liwork);

    // pulp_trace_kernel_declare(0, "kernel 0");
    // pulp_trace_kernel_start(0, 1);

    // compute and subtract mean
    for (int j = 0; j < variables; j++) {
        real mean = 0.0;
        #pragma omp parallel for reduction(+:mean)
        for (int i = 0; i < samples; i++)
            mean += input[j * samples + i];
        mean /= samples;
        #pragma omp parallel for
        for (int i = 0; i < samples; i++)
            T[j * samples + i] = input[j * samples + i] - mean;
    }

    // compute A=T^T*T
    for (int j = 0; j < variables; j++)
        for (int i = 0; i <= j; i++) {
            real dot = 0;
            #pragma omp parallel for reduction(+:dot)
            for (int k = 0; k < samples; k++)
                dot += T[j * samples + k] * T[i * samples + k];
            A[i + j * variables] = dot;
        }

    // tridiagonalization
#if defined USE_MKL || USE_LAPACK
    int info = LAPACKE_ssytrd(LAPACK_COL_MAJOR, 'U', variables, A, variables, d, e, tau);
#else
    int info = PCA_ssytd2('U', variables, A, variables, d, e, tau);
#endif
    if (info != 0) {
        printf("Error in SSYTRD/SSYTD2: %i\n", info);
        abort();
    }

    // compute eigenvalues
    int il = variables - components + 1, iu = variables, m, tryrac = 1;
    real vl = 0.0, vu = 0.0;
    info = PCA_sstemr('V', 'I', variables, d, e, vl, vu, il, iu, &m, w, Z, variables, variables,
        isuppz, &tryrac, work, lwork, iwork, liwork);
    if (info != 0) {
        printf("Error in SSTEMR: %i\n", info);
        abort();
    }
    printf("%d: ", m);
    for (int i = 0; i < m; i++) printf("%d ", (int)w[i]); printf("\n");

    // compute eigenvectors
#if defined USE_MKL || USE_LAPACK
    info = LAPACKE_sormtr(LAPACK_COL_MAJOR, 'L', 'U', 'N', variables, m, A, variables, tau,
        Z, variables);
#else
    info = PCA_sorm2l('L', 'N', variables - 1, m, variables - 1, A + variables, variables, tau,
        Z, variables, work);
#endif
    if (info != 0) {
        printf("Error in SORMTR/SORM2L: %i\n", info);
        abort();
    }

#if defined USE_ESSL || USE_MKL || USE_LAPACK
    cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, samples, components, variables,
        1.0, T, samples, Z, variables, 0.0, output, samples);
#else
    #pragma omp parallel for
    for (int i = 0; i < samples; i++)
        for (int j = 0; j < components; j++) {
            real t = 0;
            for (int k = 0; k < variables; k++)
                t += T[i + k * samples] * Z[j * variables + k];
            output[i + j * samples] = t;
        }
#endif

    FREE(T);
    FREE(A);
    FREE(e);
    FREE(d);
    FREE(tau);
    FREE(w);
    FREE(Z);
    FREE(isuppz);
    FREE(work);
    FREE(iwork);
    // pulp_trace_kernel_stop(0, 1);
}
