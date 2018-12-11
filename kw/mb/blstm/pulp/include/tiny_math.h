#ifndef TINY_MATH_H
#define TINY_MATH_H

/* Approximation of functions from:
N. N. Schraudolph, "A Fast, Compact Approximation of the Exponential Function,"
in Neural Computation, vol. 11, no. 4, pp. 853-862, May 15 1999.
doi: 10.1162/089976699300016467
URL: http://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=6790798&isnumber=6790368
Code snippet available at https://martin.ankerl.com/2007/10/04/optimized-pow-approximation-for-java-and-c-c/
*/

// for x86, power8 minsky
#define LITTLE_ENDIAN

#define EXP_A 184
#define EXP_C 16249

union eco_u
{
  float d;
  struct
  {
    #ifdef LITTLE_ENDIAN
    short j, i;
    #else
    short i, j;
    #endif
  } n;
};


inline float tiny_expf(float y)
{
  if (y < -25) // FIXME: have to find the exact value
    return 0;
  else if (y > 25)
    return FLT_MAX;
  else {
    union eco_u eco;
    eco.n.i = EXP_A*(y) + (EXP_C);
    eco.n.j = 0;
    // Special case that requested number is out of supported range (resulting in NaN)
    //assert (eco.d == eco.d);
    return eco.d;
  }
}

float LOG(float y)
{
  int * nTemp = (int*)&y;
  y = (*nTemp) >> 16;
  return (y - EXP_C) / EXP_A;
}

float POW(float b, float p)
{
  return tiny_expf(LOG(b) * p);
}

float tiny_tanhf(float x) {
  // tanh = 1-e^(-2x) / 1+e^(-2x)
  float exp_val = tiny_expf(-2 * x);
  return((1-exp_val) / (1+exp_val));
}


#endif
