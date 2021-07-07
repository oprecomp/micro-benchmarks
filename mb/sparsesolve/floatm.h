#ifndef FLOATM
#define FLOATM double
#endif

#ifndef DOUBLE
#define DOUBLE double
#endif

static inline double double_truncate(uint8_t m, double y)
{
    if (m >= 52) return y; // nothing to do
    if (!isfinite(y)) return y; // NaN and Inf does not need conversion
    union { double d; uint64_t i; } c;
    c.d = y;
    c.i &= ~(((1UL << (52 - m)) - 1UL));
    return c.d;
}

static inline float float_truncate(uint8_t m, float y)
{
    if (m >= 23) return y; // nothing to do
    if (!isfinite(y)) return y; // NaN and Inf does not need conversion
    union { float f; uint32_t i; } c;
    c.f = y;
    c.i &= ~(((1U << (23 - m)) - 1U));
    return c.f;
}

#define TRUNCATE2(t, m, y) t ## _truncate(m, y)
#define TRUNCATE(t, m, y) TRUNCATE2(t, m, y)
