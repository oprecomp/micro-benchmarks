#ifndef FLOAT
#define FLOAT float
#endif

#ifndef DOUBLE
#define DOUBLE double
#endif

#ifndef FLOAT2
#define FLOAT2 DOUBLE
#endif

#define ALLOC(t, l) (t*)malloc((l) * sizeof(t))
#define CALLOC(t, l) (t*)calloc(sizeof(t),(l))
#define FREE(p) *p = INFINITY; free(p);

