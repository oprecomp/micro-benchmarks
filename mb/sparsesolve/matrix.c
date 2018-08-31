
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <float.h>

#include "mmio.h"
#include <tgmath.h> // interferes with mmio.h

#include "floatm.h"
#include "matrix.h"

static int compar(const void *pa, const void *pb)
{
    struct matrix_coo *a = (struct matrix_coo*)pa;
    struct matrix_coo *b = (struct matrix_coo*)pb;
    if (a->i < b->i) return -1;
    if (a->i > b->i) return 1;
    if (a->j < b->j) return -1;
    if (a->j > b->j) return 1;
    fprintf(stderr, "Repeated matrix element %i %i\n", a->i, a->j);
    exit(1);
}

void coo_load(const char *fname, int *n, int *nz, struct matrix_coo **a)
{
    FILE *f;
    if ((f = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "Error opening file: %s\n", fname);
        exit(1);
    }
    MM_typecode matcode;
    if (mm_read_banner(f, &matcode) != 0) {
        fprintf(stderr, "Could not process Matrix Market banner\n");
        exit(1);
    }
    if (!(mm_is_real(matcode) && mm_is_matrix(matcode) && mm_is_sparse(matcode))) {
        fprintf(stderr, "This application does not support the Market Market type: %s\n",
                mm_typecode_to_str(matcode));
        exit(1);
    }
    int M, N, NZ;
    if (mm_read_mtx_crd_size(f, &M, &N, &NZ) != 0) {
        fprintf(stderr, "Could not parse matrix size\n");
        exit(1);
    }
    if (M != N) {
        fprintf(stderr, "Matrix is not square\n");
        exit(1);
    }
    struct matrix_coo *coo = malloc(sizeof(struct matrix_coo) * 2 * NZ);
    if (coo == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    int k = 0;
    for (int l = 0; l < NZ; l++) {
        double real;
        if (mm_read_mtx_crd_entry(f, &coo[k].i, &coo[k].j, &real, NULL, matcode) != 0) {
            fprintf(stderr, "Error reading matrix element %i\n", l);
            exit(1);
        }
        coo[k].i--;
        coo[k].j--;
        coo[k].a = real;
        if (coo[k].i == coo[k].j) k++;
        else {
            coo[k + 1].i = coo[k].j;
            coo[k + 1].j = coo[k].i;
            coo[k + 1].a = coo[k].a;
            k += 2;
        }
    }
    fclose(f);
    qsort(coo, k, sizeof(struct matrix_coo), compar);
    *n = N; *nz = k; *a = coo;
}

// CSR matrix

struct matrix_csr { struct matrix super; int *i; int *j; DOUBLE *A; };

void csr_dmult(struct matrix_csr *mat, DOUBLE *x, DOUBLE *y)
{
    for (int k = 0; k < mat->super.n; k++) {
        DOUBLE t = 0.0;
        for (int l = mat->i[k]; l < mat->i[k + 1]; l++)
            t += mat->A[l] * x[mat->j[l]];
        y[k] = t;
    }
}

void csr_smult(struct matrix_csr *mat, FLOATM *x, FLOATM *y)
{
    for (int k = 0; k < mat->super.n; k++) {
        FLOATM t = 0.0;
        for (int l = mat->i[k]; l < mat->i[k + 1]; l++)
            t += mat->A[l] * x[mat->j[l]];
        y[k] = t;
    }
}

struct matrix *csr_create(int n, int nz, struct matrix_coo *coo)
{
    int *i = malloc(sizeof(int) * (n + 1));
    int *j = malloc(sizeof(int) * nz);
    DOUBLE *A = malloc(sizeof(DOUBLE) * nz);

    i[0] = 0;
    int l = 0;
    for (int k = 0; k < n; k++) {
        while (l < nz && coo[l].i == k) {
            j[l] = coo[l].j;
            A[l] = coo[l].a;
            l++;
        }
        i[k + 1] = l;
    }

    struct matrix_csr *mat = malloc(sizeof(struct matrix_csr));
    mat->super.n = n;
    mat->i = i;
    mat->j = j;
    mat->A = A;
    mat->super.dmult = (void (*)(struct matrix *, DOUBLE *, DOUBLE *))csr_dmult;
    mat->super.smult = (void (*)(struct matrix *, FLOATM *, FLOATM *))csr_smult;
    return (struct matrix *)mat;
}

// dense matrix

struct matrix_dense { struct matrix super; DOUBLE *A; };

void dense_dmult(struct matrix_dense *mat, DOUBLE *x, DOUBLE *y)
{
    for (int i = 0; i < mat->super.n; i++) {
        DOUBLE t = 0.0;
        for (int j = 0; j < mat->super.n; j++)
            t += mat->A[i * mat->super.n + j] * x[j];
        y[i] = t;
    }
}

void dense_smult(struct matrix_dense *mat, FLOATM *x, FLOATM *y)
{
    for (int i = 0; i < mat->super.n; i++) {
        FLOATM t = 0.0;
        for (int j = 0; j < mat->super.n; j++)
            t += mat->A[i * mat->super.n + j] * x[j];
        y[i] = t;
    }
}

struct matrix *dense_create(int n, int nz, struct matrix_coo *coo)
{
    DOUBLE *A = calloc(n * n, sizeof(DOUBLE));

    // row major format
    for (int k = 0; k < nz; k++) A[coo[k].i * n + coo[k].j] = coo[k].a;

    struct matrix_dense *mat = malloc(sizeof(struct matrix_dense));
    mat->super.n = n;
    mat->super.dmult = (void (*)(struct matrix *, DOUBLE *, DOUBLE *))dense_dmult;
    mat->super.smult = (void (*)(struct matrix *, FLOATM *, FLOATM *))dense_smult;
    mat->A = A;
    return (struct matrix *)mat;
}

// Jacobi preconditioner

struct precond_jacobi { struct matrix super; int bits; FLOATM *d; };

void jacobi_smult(struct precond_jacobi *pre, FLOATM *x, FLOATM *y)
{
    for (int k = 0; k < pre->super.n; k++) {
       y[k] = TRUNCATE(FLOATM, pre->bits, x[k] / pre->d[k]);
    }
}

struct matrix *jacobi_create(int bits, int n, int nz, struct matrix_coo *coo)
{
    FLOATM *d = malloc(sizeof(FLOATM) * n);
    for (int k = 0; k < n; k++) d[k] = 0.0;
    for (int k = 0; k < nz; k++)
        if (coo[k].i == coo[k].j)
            d[coo[k].i] = TRUNCATE(FLOATM, bits, coo[k].a);

    struct precond_jacobi *pre = malloc(sizeof(struct precond_jacobi));
    pre->super.n = n;
    pre->super.dmult = NULL;
    pre->super.smult = (void (*)(struct matrix *, FLOATM *, FLOATM *))jacobi_smult;
    pre->bits = bits;
    pre->d = d;
    return (struct matrix *)pre;
}
