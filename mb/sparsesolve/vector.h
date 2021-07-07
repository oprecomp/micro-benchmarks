// BLAS like functions

static inline void vector_set(int n, DOUBLE a, DOUBLE *x) {
    for (int i = 0; i < n; i++) x[i] = a;
}

static inline void vector_xpby(int n, DOUBLE *x, DOUBLE b, DOUBLE *y) {
    for (int i = 0; i < n; i++) y[i] = x[i] + b * y[i];
}

static inline DOUBLE vector_dot(int n, DOUBLE *x, DOUBLE *y) {
    DOUBLE r = 0.0;
    for (int i = 0; i < n; i++) r += y[i] * x[i];
    return r;
}

static inline DOUBLE vector_norm2(int n, DOUBLE *x) {
    return sqrt(vector_dot(n, x, x));
}

// limited precision version

static inline void floatm_set(uint8_t m, int n, FLOATM a, FLOATM *x) {
    FLOATM b = TRUNCATE(FLOATM, m, a);
    for (int i = 0; i < n; i++) x[i] = b;
}

static inline void floatm_copy(uint8_t m, int n, FLOATM *x, FLOATM *y) {
    for (int i = 0; i < n; i++) y[i] = TRUNCATE(FLOATM, m, x[i]);
}

static inline void floatm_axpy(uint8_t m, int n, FLOATM a, FLOATM *x, FLOATM *y) {
    for (int i = 0; i < n; i++) y[i] = TRUNCATE(FLOATM, m, y[i] + x[i] * a);
}

static inline void floatm_xpby(uint8_t m, int n, FLOATM *x, FLOATM b, FLOATM *y) {
    for (int i = 0; i < n; i++) y[i] = TRUNCATE(FLOATM, m, x[i] + b * y[i]);
}

// the dot product is computed with full precision

static inline FLOATM floatm_dot(int n, FLOATM *x, FLOATM *y) {
    FLOATM r = 0.0;
    for (int i = 0; i < n; i++) r += y[i] * x[i];
    return r;
}

static inline FLOATM floatm_norm2(int n, FLOATM *x) {
    return sqrt(floatm_dot(n, x, x));
}

// mixed precision routines

static inline void vector_copy(uint8_t m, int n, DOUBLE *x, FLOATM *y) {
    for (int i = 0; i < n; i++) y[i] = TRUNCATE(FLOATM, m, x[i]);
}

static inline void vector_axpy(int n, DOUBLE a, FLOATM *x, DOUBLE *y) {
    for (int i = 0; i < n; i++) y[i] += x[i] * a;
}
