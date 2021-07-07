// definitions for matrices

struct matrix_coo { int i; int j; double a; };

extern void coo_load(const char *fname, int *n, int *nz, struct matrix_coo **coo);

struct matrix {
    int n;
    void (*dmult)(struct matrix *, DOUBLE *, DOUBLE *);
    void (*smult)(struct matrix *, FLOATM *, FLOATM *);
};

static inline void matrix_mult(struct matrix *mat, DOUBLE *x, DOUBLE *y) {
    mat->dmult(mat, x, y);
}

static inline void floatm_mult(struct matrix *mat, FLOATM *x, FLOATM *y) {
    mat->smult(mat, x, y);
}

extern struct matrix *csr_create(int n, int nz, struct matrix_coo *coo);

extern struct matrix *dense_create(int n, int nz, struct matrix_coo *coo);

extern struct matrix *jacobi_create(int bits, int n, int nz, struct matrix_coo *coo);
