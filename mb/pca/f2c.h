#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

// reduced definitions from f2c

typedef int integer;
typedef int ftnlen;
typedef int logical;

#ifdef FIXED_POINT

#include "fixed_point.h"

// typedef fixed_point<int32_t, int64_t, 16> real;
typedef fixed_point<int64_t, __int128, 32> real;

#else

typedef float real;

#endif

typedef double doublereal;

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define dabs(x) (doublereal)abs(x)
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define dmin(a,b) (doublereal)min(a,b)
#define dmax(a,b) (doublereal)max(a,b)

#define TRUE_ (1)
#define FALSE_ (0)

static inline double r_sign(real *a, real *b)
{
    double x;
    x = (*a >= 0 ? *a : - *a);
    return( *b >= 0 ? x : -x);
}

// reduced run-time for blas/lapack routines

static inline int xerbla_(char *srname, integer *info, ftnlen l)
{
    printf(" ** On entry to %*s parameter number %i had an illegal value\n", l, srname, *info);
    abort();
}

static inline logical lsame_(char *a, char *b, ftnlen la, ftnlen lb)
{
    return *a == *b;
}

static inline logical sisnan_(real *s)
{
#ifndef FIXED_POINT
    if (isnan(*s)) return TRUE_;
    else
#endif
    return FALSE_;
}

static inline real slamch_(char *cmach, ftnlen l)
{
#ifdef FIXED_POINT
    const real eps = real::fixed_min(); // EPSILON(ZERO) * 0.5
    const real sfmin = real::fixed_min(); // TINY(ZERO)
    const real small = 1.0 / real::fixed_max(); // HUGE(ZERO)
    const real safe = small >= sfmin ? small * (1.0 + eps) : sfmin;
    switch (*cmach) {
    case 'E':
        return eps;
    case 'S':
        return safe;
    case 'B':
        return 2; // RADIX(ZERO)
    case 'P':
        return eps * 2; // RADIX(ZERO)
    case 'N':
        return real::fixed_digits(); // DIGITS(ZERO)
    case 'R':
        return 1.0;
    case 'M':
        return 0; // MINEXPONENT(ZERO)
    case 'U':
        return real::fixed_min(); // tiny(zero)
    case 'L':
        return 0; // MAXEXPONENT(ZERO)
    case 'O':
        return real::fixed_max(); // HUGE(ZERO)
    default:
        return 0;
    }
#else
    const real eps = FLT_EPSILON * 0.5; // EPSILON(ZERO) * 0.5
    const real sfmin = FLT_MIN; // TINY(ZERO)
    const real small = 1.0 / FLT_MAX; // HUGE(ZERO)
    const real safe = small >= sfmin ? small * (1.0 + eps) : sfmin;
    switch (*cmach) {
    case 'E':
        return eps;
    case 'S':
        return safe;
    case 'B':
        return FLT_RADIX; // RADIX(ZERO)
    case 'P':
        return eps * FLT_RADIX; // RADIX(ZERO)
    case 'N':
        return FLT_MANT_DIG; // DIGITS(ZERO)
    case 'R':
        return 1.0;
    case 'M':
        return FLT_MIN_EXP; // MINEXPONENT(ZERO)
    case 'U':
        return FLT_MIN; // tiny(zero)
    case 'L':
        return FLT_MAX_EXP; // MAXEXPONENT(ZERO)
    case 'O':
        return FLT_MAX; // HUGE(ZERO)
    default:
        return 0;
    }
#endif
}

// BLAS replacement functions using extended precision for fixed point

#ifdef FIXED_POINT
static inline real sdot_(integer *n, real *sx, integer *incx, real *sy, integer *incy) {
    return real::dot(*n, sx, *incx, sy, *incy);
}

int slassq_(integer *n, real *x, integer *incx, real *scale, real *sumsq);

static inline real snrm2_(integer *n, real *x, integer *incx) {
    // return sqrt(real::ldot(*n, x, *incx, x, *incx));
    real scale = 0;
    real ssq = 1;
    slassq_(n, x, incx, &scale, &ssq);
    return scale * sqrt(ssq);
}

static inline int sgemv_(char *trans, integer *m, integer *n, real *alpha, real *a, integer *lda, real *x, integer *incx, real *beta, real *y, integer *incy, ftnlen trans_len) {
    if (*trans == 'T') {
        for (int i = 0; i < *n; i++) {
            *y = *y * *beta + *alpha * real::dot(*m, a, 1, x, *incx);
            a += *lda;
            y += *incy;
        }
    } else {
        for (int i = 0; i < *m; i++) {
            *y = *y * *beta + *alpha * real::dot(*n, a, *lda, x, *incx);
            a++;
            y += *incy;
        }
    }
}

static inline int ssymv_(char *uplo, integer *n, real *alpha, real *a, integer *lda, real *x, integer *incx, real *beta, real *y, integer *incy, ftnlen uplo_len) {
    if (*uplo == 'U') {
        for (int i = 0; i < *n; i++) {
            *y = *y * *beta + *alpha * (real::dot(i, a + *lda * i, 1, x, *incx) +
                real::dot(*n - i, a + *lda * i + i, *lda, x + i, *incx));
            y += *incy;
        }
    } else {
        for (int i = 0; i < *n; i++) {
            *y = *y * *beta + *alpha * (real::dot(i, a + i, *lda, x, *incx) +
                real::dot(*n - i, a + *lda * i + i, 1, x + i, *incx));
            y += *incy;
        }
    }
}

#endif
