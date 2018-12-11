#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <tgmath.h>
#include <float.h>
#include <stdbool.h>

#include "cg.h"
#include "vector.h"
#include "matrix.h"

void conjugate_gradient(int n, struct matrix *A, struct matrix *M, FLOAT *b, FLOAT *x, int maxiter, FLOAT umbral, int step_check, int *in_iter)
{
    int iter = 0;
    FLOAT2 alpha, beta, rho, tau, tol;

    FLOAT *r = ALLOC(FLOAT, n);
    FLOAT *p = ALLOC(FLOAT, n);
    FLOAT *z = ALLOC(FLOAT, n);

    FLOAT *tr = ALLOC(FLOAT, n);

    floatm_mult(A, x, r);
    floatm_xpby(n, b, -1.0, r); // r = b - Ax

    if (M) {
        floatm_mult(M, r, p);
        rho = floatm_dot(n, r, p);
        tol = floatm_norm2(n, r);
    } else { // small optimization
        floatm_copy(n, r, p);
        rho = floatm_dot(n, r, r);
        tol = sqrt(rho);
    }

    int step = 0;
    FLOAT2 residual = tol;

    while ((iter < maxiter) && (tol > umbral)) {
        // alpha = (r,z) / (Ap,p)
        // for (int i = 0; i < n; i++) p[i] = double(p[i]);
        floatm_mult(A, p, z);
        // compute true residual
        if (step < step_check) step++;
        else {
            floatm_mult(A, x, tr);
            floatm_xpby(n, b, -1.0, tr);
            residual = floatm_norm2(n, tr);
            printf("# rescheck: %d %d %e %e\n", *in_iter, iter, (double)tol, (double)residual);
            if (residual / tol > 10) break;
            step = 1;
        }

        alpha = rho / floatm_dot(n, z, p);
        // x = x + alpha * p
        floatm_axpy(n, alpha, p, x);
        // r = r - alpha * Ap
        floatm_axpy(n, -alpha, z, r);
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
        if (M) floatm_xpby(n, z, beta, p);
        else floatm_xpby(n, r, beta, p);
        iter++;
        (*in_iter)++;
        /* printf ("%d %d %d %e %e\n", *in_iter, iter, bits, (double)tol, (double)residual);
        printf("alpha %e %e\n", alpha.value(), alpha.error());
        printf("beta %e %e\n", beta.value(), beta.error());
        printf("rho %e %e\n", rho.value(), rho.error());
        printf("tau %e %e\n", tau.value(), tau.error()); */

    }

    FREE(r);
    FREE(p);
    FREE(z);
    FREE(tr);
}
