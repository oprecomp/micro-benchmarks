#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <tgmath.h>
#include <float.h>

#include "floatm.h"
#include "vector.h"
#include "matrix.h"
#include "oprecomp.h"

// #define USE_DENSE
#define USE_PRECOND

void iterative_refinement(int n, struct matrix *A, struct matrix *M, DOUBLE *b, DOUBLE *x,
        int out_maxiter, DOUBLE out_tol, int in_maxiter, DOUBLE in_tol, int step_check,
        int *out_iter, int *in_iter);

// #define USE_DENSE
// #define USE_PRECOND

int conjugate_gradient(uint8_t bits, int n, struct matrix *A, struct matrix *M, FLOATM *b, FLOATM *x, int maxiter, FLOATM umbral)
{
    int iter = 0;
    FLOATM alpha, beta, rho, tau, tol;

    FLOATM *r = ALLOC(FLOATM, n);
    FLOATM *p = ALLOC(FLOATM, n);
    FLOATM *z = ALLOC(FLOATM, n);

    floatm_set(bits, n, 1.0, x);
    floatm_mult(A, x, r);
    floatm_xpby(bits, n, b, -1.0, r); // r = b - Ax

    if (M) {
        floatm_mult(M, r, p);
        rho = floatm_dot(n, r, p);
        tol = floatm_norm2(n, r);
    } else { // small optimization
        floatm_copy(bits, n, r, p);
        rho = floatm_dot(n, r, r);
        tol = sqrt(rho);
    }

    while ((iter < maxiter) && (tol > umbral)) {
        // alpha = (r,z) / (Ap,p)
        floatm_mult(A, p, z);
        alpha = rho / floatm_dot(n, z, p);
        // x = x + alpha * p
        floatm_axpy(bits, n, alpha, p, x);
        // r = r - alpha * Ap
        floatm_axpy(bits, n, -alpha, z, r);
        // apply preconditioner
        if (M) {
            floatm_mult(M, r, z);
            tau = floatm_dot(n, r, z);
            tol = floatm_norm2(n, r);
        } else {
            tau = floatm_dot(n, r, r);
            tol = sqrt(tau);
        }
        // beta = (r,z) / rho
        beta =  tau / rho;
        rho = tau;
        // p = z + beta * p
        if (M) floatm_xpby(bits, n, z, beta, p);
        else floatm_xpby(bits, n, r, beta, p);
        iter++;
        // printf ("(%d,%20.10e)\n", iter, tol);
    }

    free(r);
    free(p);
    free(z);
    return iter;
}

int main (int argc, char *argv[])
{
    if (argc != 7) {
        fprintf(stderr, "Missing arguments: matrix_file out_its out_tol in_its in_tol mantissa_bits\n");
        return 1;
    }

    int n, nz;
    struct matrix_coo *coo;
    coo_load(argv[1], &n, &nz, &coo);

#ifdef USE_DENSE
    struct matrix *A = dense_create(n, nz, coo);
#else
    struct matrix *A = csr_create(n, nz, coo);
#endif

    DOUBLE *x = ALLOC(DOUBLE, n);
    DOUBLE *b = ALLOC(DOUBLE, n);
    DOUBLE *e = ALLOC(DOUBLE, n);
    FLOATM *r = ALLOC(FLOATM, n);
    FLOATM *d = ALLOC(FLOATM, n);
    vector_set(n, 1.0, b);

    int out_maxiter = atoi(argv[2]);
    DOUBLE out_tol = atof(argv[3]);
    if (out_tol <= 0.0) out_tol = 1.0e-8;

    int in_maxiter = atoi(argv[4]);
    DOUBLE in_tol = atof(argv[5]);
    if (in_tol <= 0.0) in_tol = 1.0e-8;

    uint8_t bits = atoi(argv[6]);

#ifdef USE_PRECOND
    struct matrix *M = jacobi_create(bits, n, nz, coo);
#else
    struct matrix *M = NULL;
#endif
    free(coo);

    int out_iter, in_iter;
    DOUBLE residual;

    oprecomp_start();
    do {

    out_iter = 0; in_iter = 0;
    vector_set(n, 0.0, x);
    vector_copy(bits, n, b, r);

    do {
        in_iter += conjugate_gradient(bits, n, A, M, r, d, in_maxiter, in_tol);

        vector_axpy(n, 1.0, d, x); // x = x + d
        matrix_mult(A, x, e);
        vector_xpby(n, b, -1.0, e);
        residual = vector_norm2(n, e);
        vector_copy(bits, n, e, r); // r = b - Ax

        out_iter++;
        printf("[%i] %e\n", in_iter, residual);
    } while ((residual > out_tol) && (out_iter < out_maxiter));

    } while (oprecomp_iterate());
    oprecomp_stop();

    printf("# matrix: %s\n", argv[1]);
    printf("# problem_size: %d\n", n);
    printf("# outer_maxiter: %d\n", out_maxiter);
    printf("# outer_tolerance: %e\n", out_tol);
    printf("# inner_maxiter: %d\n", in_maxiter);
    printf("# inner_tolerance: %e\n", in_tol);
    printf("# precision: %d\n", bits);
    printf("# outer_iterations: %d\n", out_iter);
    printf("# inner_iterations: %d\n", in_iter);
    printf("# residual: %e\n", residual);
}
