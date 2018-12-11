// Template class for general fixed point arithmetic

template <typename I, typename L, int D>
class fixed_point {
private:
    I v;
public:
    fixed_point<I,L,D>() { }

    fixed_point<I,L,D>(float f) { v = roundf(f * (I(1) << D)); }
    operator float() { return float(v) / (I(1) << D); }

    fixed_point<I,L,D>(double d) { v = round(d * (I(1) << D)); }
    operator double() { return double(v) / (I(1) << D); }

    fixed_point<I,L,D>(int i) { v = I(i) << D; }
    operator int() { return v >> D; }

    I raw() { return v; }
    static fixed_point<I,L,D> raw(I i) { fixed_point<I,L,D> r; r.v = i; return r; }
    static fixed_point<I,L,D> fixed_min() { return raw(1); }
    static fixed_point<I,L,D> fixed_max() { return raw(~(I(1) << (sizeof(I) * 8 - 1))); }
    static int fixed_digits() { return sizeof(I) * 8; }

    fixed_point<I,L,D> operator -() { fixed_point<I,L,D> r; r.v = -v; return r; }

    friend fixed_point<I,L,D> operator +(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return raw(a.v + b.v); }
    friend fixed_point<I,L,D> operator -(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return raw(a.v - b.v); }
    friend fixed_point<I,L,D> operator *(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return raw((L(a.v) * L(b.v)) >> D); }
    friend fixed_point<I,L,D> operator /(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return raw((L(a.v) << D) / b.v); }

    static fixed_point<I,L,D> dot(int n, fixed_point<I,L,D> *a, int ia, fixed_point<I,L,D> *b, int ib) {
    #if 0
        fixed_point<I,L,D> dot = 0;
        for (int i = 0; i < n; i++) {
            dot += *a * *b;
            a += ia; b += ib;
        }
        return dot;
    #else
        L dot = 0;
        for (int i = 0; i < n; i++) {
            dot += (L(a->v) * L(b->v));
            a += ia; b += ib;
        }
        return raw(dot >> D);
    #endif
    }

    fixed_point<I,L,D> operator +=(fixed_point<I,L,D> a) { v += a.v; return *this; }
    fixed_point<I,L,D> operator -=(fixed_point<I,L,D> a) { v -= a.v; return *this; }
    fixed_point<I,L,D> operator *=(fixed_point<I,L,D> a) { v = (L(v) * L(a.v)) >> D; return *this; }
    fixed_point<I,L,D> operator /=(fixed_point<I,L,D> a) { v = (L(v) << D) / a.v; return *this; }

    friend bool operator ==(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v == b.v; }
    friend bool operator !=(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v != b.v; }
    friend bool operator < (fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v <  b.v; }
    friend bool operator > (fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v >  b.v; }
    friend bool operator <=(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v <= b.v; }
    friend bool operator >=(fixed_point<I,L,D> a, fixed_point<I,L,D> b) { return a.v >= b.v; }

    friend fixed_point<I,L,D> operator +(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) + b; }
    friend fixed_point<I,L,D> operator -(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) - b; }
    friend fixed_point<I,L,D> operator *(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) * b; }
    friend fixed_point<I,L,D> operator /(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) / b; }

    friend fixed_point<I,L,D> operator +(fixed_point<I,L,D> a, float b) { return a + fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator -(fixed_point<I,L,D> a, float b) { return a - fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator *(fixed_point<I,L,D> a, float b) { return a * fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator /(fixed_point<I,L,D> a, float b) { return a / fixed_point<I,L,D>(b); }

    friend fixed_point<I,L,D> operator +(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) + b; }
    friend fixed_point<I,L,D> operator -(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) - b; }
    friend fixed_point<I,L,D> operator *(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) * b; }
    friend fixed_point<I,L,D> operator /(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) / b; }

    friend fixed_point<I,L,D> operator +(fixed_point<I,L,D> a, double b) { return a + fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator -(fixed_point<I,L,D> a, double b) { return a - fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator *(fixed_point<I,L,D> a, double b) { return a * fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator /(fixed_point<I,L,D> a, double b) { return a / fixed_point<I,L,D>(b); }

    friend fixed_point<I,L,D> operator +(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) + b; }
    friend fixed_point<I,L,D> operator -(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) - b; }
    friend fixed_point<I,L,D> operator *(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) * b; }
    friend fixed_point<I,L,D> operator /(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) / b; }

    friend fixed_point<I,L,D> operator +(fixed_point<I,L,D> a, int b) { return a + fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator -(fixed_point<I,L,D> a, int b) { return a - fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator *(fixed_point<I,L,D> a, int b) { return a * fixed_point<I,L,D>(b); }
    friend fixed_point<I,L,D> operator /(fixed_point<I,L,D> a, int b) { return a / fixed_point<I,L,D>(b); }

    friend bool operator ==(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) == b; }
    friend bool operator !=(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) != b; }
    friend bool operator < (float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <  b; }
    friend bool operator > (float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >  b; }
    friend bool operator <=(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <= b; }
    friend bool operator >=(float a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >= b; }

    friend bool operator ==(fixed_point<I,L,D> a, float b) { return a == fixed_point<I,L,D>(b); }
    friend bool operator !=(fixed_point<I,L,D> a, float b) { return a != fixed_point<I,L,D>(b); }
    friend bool operator < (fixed_point<I,L,D> a, float b) { return a <  fixed_point<I,L,D>(b); }
    friend bool operator > (fixed_point<I,L,D> a, float b) { return a >  fixed_point<I,L,D>(b); }
    friend bool operator <=(fixed_point<I,L,D> a, float b) { return a <= fixed_point<I,L,D>(b); }
    friend bool operator >=(fixed_point<I,L,D> a, float b) { return a >= fixed_point<I,L,D>(b); }

    friend bool operator ==(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) == b; }
    friend bool operator !=(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) != b; }
    friend bool operator < (double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <  b; }
    friend bool operator > (double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >  b; }
    friend bool operator <=(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <= b; }
    friend bool operator >=(double a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >= b; }

    friend bool operator ==(fixed_point<I,L,D> a, double b) { return a == fixed_point<I,L,D>(b); }
    friend bool operator !=(fixed_point<I,L,D> a, double b) { return a != fixed_point<I,L,D>(b); }
    friend bool operator < (fixed_point<I,L,D> a, double b) { return a <  fixed_point<I,L,D>(b); }
    friend bool operator > (fixed_point<I,L,D> a, double b) { return a >  fixed_point<I,L,D>(b); }
    friend bool operator <=(fixed_point<I,L,D> a, double b) { return a <= fixed_point<I,L,D>(b); }
    friend bool operator >=(fixed_point<I,L,D> a, double b) { return a >= fixed_point<I,L,D>(b); }

    friend bool operator ==(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) == b; }
    friend bool operator !=(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) != b; }
    friend bool operator < (int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <  b; }
    friend bool operator > (int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >  b; }
    friend bool operator <=(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) <= b; }
    friend bool operator >=(int a, fixed_point<I,L,D> b) { return fixed_point<I,L,D>(a) >= b; }

    friend bool operator ==(fixed_point<I,L,D> a, int b) { return a == fixed_point<I,L,D>(b); }
    friend bool operator !=(fixed_point<I,L,D> a, int b) { return a != fixed_point<I,L,D>(b); }
    friend bool operator < (fixed_point<I,L,D> a, int b) { return a <  fixed_point<I,L,D>(b); }
    friend bool operator > (fixed_point<I,L,D> a, int b) { return a >  fixed_point<I,L,D>(b); }
    friend bool operator <=(fixed_point<I,L,D> a, int b) { return a <= fixed_point<I,L,D>(b); }
    friend bool operator >=(fixed_point<I,L,D> a, int b) { return a >= fixed_point<I,L,D>(b); }
};
