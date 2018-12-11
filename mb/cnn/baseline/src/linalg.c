/*
 * linalg.c
 * Francesco Conti <f.conti@unibo.it>
 *
 * Copyright (C) 2015 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "linalg.h"
#ifdef IPC_CONV16
  #include "perf_monitor.h"
#elif IPC_CONV16_INNER
  #include "perf_monitor.h"
#endif
#ifdef CCN_HWCE_ACCEL
  #include "hwce.h"
#endif

#ifndef PULP_CHIP
    #define PULP_CHIP -1
#endif

unsigned int hwce_iter = 0;

/**
 *  @brief Various versions of the 2d convolution.
 *
 *  Computes the 2d convolution y=W*x (inlined). The baseline
 *  "algorithmic" version uses nested loops over the output pixel (i,j)
 *  and the filter tap (ui,uj).
 *
 *  @param *W
 *      a pointer to the base of the 4d weight tensor. Weights are organized
 *      in a W[a,b,i,j] fashion, where a is the index of the output feature
 *      map, b is the index of the input feature map, and (i,j) are the
 *      coordinates of a pixel in the filter. Data layout on memory is
 *      row-major. The __restrict__ keyword is used to indicate the compiler
 *      that W, x, y do not overlap.
 *  @param *x
 *      a pointer to the base of the input 3d tensor (a collection of feature
 *      maps). The inputs are organized in a x[b,i,j] fashion, where b is the
 *      index of the input feature map, and (i,j) are the coordinates of a
 *      pixel in the input feature map. The __restrict__ keyword is used to
 *      indicate the compiler that W, x, y do not overlap.
 *  @param *y
 *      a pointer to the base of the output 3d tensor (a collection of feature
 *      maps. The outputs are organized in a y[a,i,j] fashion, where a is the
 *      index of the output feature map, and (i,j) are the coordinates of a
 *      pixel in the output feature map. The __restrict__ keyword is used to
 *      indicate the compiler that W, x, y do not overlap.
 *  @param h
 *      the height of the input feature maps.
 *  @param w
 *      the width of the input feature maps.
 *  @param fs
 *      the size of the filters.
 *  @param oh
 *      the height of the output feature maps.
 *  @param ow
 *      the width of the output feature maps.
 *  @param nif
 *      the number of input feature maps.
 *  @param a
 *      the index of the output feature map.
 *  @param b
 *      the index of the input feature map.
 */

#ifdef SUPERDETAILED_DEBUG
static int64_t bigconv = 0;
#endif /* SUPERDETAILED_DEBUG */

#ifdef CCN_PULP_HWCE
int32_t conv16_hwce_11x11_async(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ y_ptr,
   int h,
   int w,
   int nif,
   int a,
   unsigned qf
) {
   hwce_conv_5x5_16x16(W_base,            x_base,        y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 1*nif*28, x_base+6,      y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 2*nif*28, x_base+12,     y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 3*nif*28, x_base+6*w,    y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 4*nif*28, x_base+6*w+6,  y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 5*nif*28, x_base+6*w+12, y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 6*nif*28, x_base+12*w,   y_ptr, h-6, w-6, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 7*nif*28, x_base+12*w+6, y_ptr, h-6, w-6, nif, a, qf);
   int id = hwce_conv_5x5_16x16_async(W_base + 8*nif*28, x_base+12*w+12, y_ptr, h-6, w-6, nif, a, qf);
   return id;
}

int32_t conv16_hwce_7x7_async(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ y_ptr,
   int h,
   int w,
   int nif,
   int a,
   unsigned qf
) {
   hwce_conv_5x5_16x16(W_base,            x_base,     y_ptr, h-2, w-2, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 1*nif*28, x_base+5,   y_ptr, h-2, w-2, nif, a, qf);
   hwce_conv_5x5_16x16(W_base + 2*nif*28, x_base+5*w, y_ptr, h-2, w-2, nif, a, qf);
   int id = hwce_conv_5x5_16x16_async(W_base + 3*nif*28, x_base+5*w+5, y_ptr, h-2, w-2, nif, a, qf);
   return id;
}

int32_t conv16_hwce_5x5_async(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ y_ptr,
   int h,
   int w,
   int nif,
   int a,
   unsigned qf
) {
   int id = hwce_conv_5x5_16x16_async(W_base, x_base, y_ptr, h, w, nif, a, qf);
   return id;
}

int32_t conv16_hwce_3x3_async(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ y_ptr,
   int h,
   int w,
   int nif,
   int a,
   unsigned qf
) {
   int id = hwce_conv_5x5_16x16_async(W_base, x_base-w-1, y_ptr, h+2, w+2, nif, a, qf);
   return id;
}
#endif /* CCN_PULP_HWCE */

static inline int32_t conv16_unrolled_ptr_loopbody(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ y_ptr,
   int w,
   int ow,
   int fs,
   int i,
   int j,
   unsigned qf
) {

   int32_t conv = 0;
   int k,l,t;

   int16_t *W_ptr = W_base;
   int16_t *x_ptr = x_base + i*w + j;

   for(k=0; k<fs; k++) {
      for(l=0; l<fs; l++) {
#ifdef SUPERDETAILED_DEBUG

#define sign_ext_4(x) \
  (int32_t) ((x >> 3) ? (0xfffffff0 | x) : (0x00000000 | x))

           int16_t w0 =  (*(W_ptr)) & 0x000f;
           int16_t w1 = ((*(W_ptr)) & 0x00f0) >> 4;
           int16_t w2 = ((*(W_ptr)) & 0x0f00) >> 8;
           int16_t w3 = ((*(W_ptr)) & 0xf000) >> 12;

           int32_t temp3 = sign_ext_4(w3) * (int32_t)(*x_ptr);
           int32_t temp2 = w2 * (int32_t)(*x_ptr);
           int32_t temp1 = w1 * (int32_t)(*x_ptr);
           int32_t temp0 = w0 * (int32_t)(*x_ptr);

         printf("    (%d,%d): (%08x,%08x,%08x,%08x) = %04x * %04x\n", k, l, temp3, temp2, temp1, temp0, (*W_ptr) & 0xffff, (*x_ptr) & 0xffff);
#endif /* SUPERDETAILED_DEBUG */

         conv += *W_ptr++ * *x_ptr--;
      }
      x_ptr -= w-fs;
   }

#ifdef SUPERDETAILED_DEBUG
   bigconv = conv;
#endif

   conv >>= qf;

   return conv;

}

static inline void conv16_unrolled_ptr(
   int16_t *__restrict__ W_base,
   int16_t *__restrict__ x_base,
   int16_t *__restrict__ y_ptr,
   int w,
   int oh,
   int ow,
   int fs,
   int parallel_type,
   unsigned qf
) {
   register int i;
   register int j;
#ifdef FULL_PRECISION
   int conv = 0;
#else
   int16_t conv = 0;
#endif

#ifdef IPC_CONV16
   unsigned int cc=0, sc=0, ic=0, dc=0;
   perf_start();
   perf_clear();
#elif IPC_CONV16_INNER
   unsigned int cc=0, sc=0, ic=0, dc=0;
#endif

   if(parallel_type == PARALLEL_PIXEL) {
#ifndef SUPERDETAILED_DEBUG
      // #pragma omp for nowait
#else
      #pragma omp master
#endif /* ~SUPERDETAILED_DEBUG */
      for (i=0; i<oh; i++) {
         for (j=0; j<ow; j++) {

            conv = conv16_unrolled_ptr_loopbody(W_base, x_base, y_ptr, w, ow, fs, i, j, qf);

#ifdef SUPERDETAILED_DEBUG
            printf("(%d,%d): %08x = %08x + %08x\n", i, j, (*(y_ptr + i*ow+j)+conv), (*(y_ptr + i*ow+j)), bigconv);
#endif /* SUPERDETAILED_DEBUG */


#ifdef FULL_PRECISION
#ifndef DONT_SATURATE
            // SAT-
            conv += *(y_ptr + i*ow+j);
            if(conv > +32767) {
#ifdef SUPERDETAILED_DEBUG
               printf("SAT+! conv=%08x\n", conv);
#endif /* SUPERDETAILED_DEBUG */
               conv = 0x00007fff;
            }
            // SAT+
            else if(conv < -32768) {
#ifdef SUPERDETAILED_DEBUG
               printf("SAT-! conv=%08x\n", conv);
#endif /* SUPERDETAILED_DEBUG */
               conv = 0xffff8000;
            }
            *(y_ptr + i*ow+j) = conv;
#else /* DONT_SATURATE */
            *(y_ptr + i*ow+j) += conv;
#endif /* DONT_SATURATE */
#else /* ~FULL_PRECISION */
            *(y_ptr + i*ow+j) += conv;
#endif /* ~FULL_PRECISION */

         }
      }
   }
   else {
      for (i=0; i<oh; i++) {
         for (j=0; j<ow; j++) {

            conv16_unrolled_ptr_loopbody(W_base, x_base, y_ptr, w, ow, fs, i, j, qf);

#ifdef SUPERDETAILED_DEBUG
            printf("(%d,%d): %08x = %08x + %08x\n", i, j, (*(y_ptr + i*ow+j)+conv), (*(y_ptr + i*ow+j)), bigconv);
#endif /* SUPERDETAILED_DEBUG */

            #pragma omp critical
            *(y_ptr + i*ow+j) += conv;

         }
      }
   }

}


static inline void conv16_approx_5x5_critical(
   int16_t  * __restrict__ y_ptr,
   int16_t  * __restrict__ conv,
   int                     ow,
   int                     i,
   int                     j
) {

   int16_t y_in[2];
   unsigned *y_in_big = (unsigned *) (&y_in[0]);

   // both versions introduce a small approximation

// #ifndef CONV_APPROX_SINGLE_LDST
    y_in[0] = *(y_ptr + i*ow+j);
    y_in[1] = *(y_ptr + i*ow+j+1);

    y_in[0] += conv[0];
    y_in[1] += conv[1];

    if(ow-j > 1) {
      *(y_ptr + i*ow+j)     = y_in[0];
      *(y_ptr + i*ow+j + 1) = y_in[1];
   }
    else
      *(y_ptr + i*ow+j) = y_in[0];
// #else
//    *y_in_big = *(unsigned *)(y_ptr + i*ow+j);
//
//    y_in[1] += conv[0];
//    y_in[0] += conv[1];
//
//    if(ow-j > 1)
//      *(unsigned *)(y_ptr + i*ow+j) = *y_in_big;
//    else
//      *(y_ptr + i*ow+j) = y_in[1];
// #endif

}

static inline void conv16_optimized_5x5_loopbody(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ conv,
   int w,
   int ow,
   int i,
   int j
) {

   int k,l;

   int16_t *W_ptr = W_base;
   int16_t *x_ptr = x_base + i*w + j;

   for(k=0; k<5; k++) {
      for(l=0; l<5; l++) {
         register int16_t  W_loc = *W_ptr++;
         // unsigned x_loc = *((unsigned *) (x_ptr-1));
         unsigned x_loc = *((unsigned *) x_ptr);
         x_ptr--;
         conv[0] += (W_loc * ((int16_t *) &x_loc)[1]) >> QF;
         conv[1] += (W_loc * ((int16_t *) &x_loc)[0]) >> QF;
      }
      x_ptr -= w-5;
   }

}

static inline void conv16_approx_5x5_loopbody(
   int16_t * __restrict__ W_base,
   int16_t * __restrict__ x_base,
   int16_t * __restrict__ conv,
   int w,
   int ow,
   int i,
   int j
) {

   int k,l;

   int16_t *W_ptr = W_base;
   int16_t *x_ptr = x_base + i*w + j;

   register vect32_t conv_big = {0,0};

   for(k=0; k<5; k++) {
      for(l=0; l<5; l++) {

         register int16_t  W_loc = *W_ptr++;
         register vect16_t x_loc = *((vect16_t *) x_ptr--);
         conv_big[0] += W_loc * x_loc[0];
         conv_big[1] += W_loc * x_loc[1];

      }
      x_ptr -= w-5;
   }

   conv[0] = conv_big[0] >> QF;
   conv[1] = conv_big[1] >> QF;

}

static inline void conv16_approx_5x5(
   int16_t *__restrict__ W_base,
   int16_t *__restrict__ x_base,
   int16_t *__restrict__ y_ptr,
   int w,
   int oh,
   int ow,
   int parallel_type
) {
   register int i,j,k,l;
   int16_t conv[2] = {0,0};
   int16_t y_in[2];
   unsigned *y_in_big = (unsigned *) (&y_in[0]);

   if(parallel_type == PARALLEL_PIXEL) {
      // #pragma omp for nowait
      for (i=0; i<oh; i++) {
         for (j=0; j<ow; j+=2) {

            conv16_approx_5x5_loopbody(W_base, x_base, conv, w, ow, i, j);
#if 0
            conv16_approx_5x5_critical(y_ptr, conv, w, i, j);
#else
            {
               y_in[0] = *(y_ptr + i*ow+j);
               y_in[1] = *(y_ptr + i*ow+j+1);

               y_in[0] += conv[0];
               y_in[1] += conv[1];

               if(ow-j > 1) {
                 *(y_ptr + i*ow+j)     = y_in[0];
                 *(y_ptr + i*ow+j + 1) = y_in[1];
              }
               else {
                 *(y_ptr + i*ow+j) = y_in[0];
               }
            }
#endif

         }
      }
   }
   else {
      for (i=0; i<oh; i++) {
         for (j=0; j<ow; j+=2) {

            conv16_approx_5x5_loopbody(W_base, x_base, conv, w, ow, i, j);
            #pragma omp critical
#if 0
            conv16_approx_5x5_critical(y_ptr, conv, w, i, j);
#else
            {
               y_in[0] = *(y_ptr + i*ow+j);
               y_in[1] = *(y_ptr + i*ow+j+1);

               y_in[0] += conv[0];
               y_in[1] += conv[1];

               if(ow-j > 1) {
                 *(y_ptr + i*ow+j)     = y_in[0];
                 *(y_ptr + i*ow+j + 1) = y_in[1];
              }
               else
                 *(y_ptr + i*ow+j) = y_in[0];
            }
#endif

         }
      }
   }

}

static inline void conv16_gold(int16_t *__restrict__ W, int16_t *__restrict__ x, int16_t *__restrict__ y, int h, int w, int fs, int oh, int ow, int nif, int a, int b) {
   int i;
   // #pragma omp for schedule(static)
   for (i=0; i<oh; i++) {
      int j;
      for (j=0; j<ow; j++) {
         int ui;
         int16_t conv = 0;
         for (ui=0; ui<fs; ui++) {
            int uj;
            for (uj=0; uj<fs; uj++) {
               int m;
               int n;
               m = i-ui+fs-1;
               n = j-uj+fs-1;
               conv += ccn_mult(W[((((a*nif)+b)*fs)+ui)*fs+uj] , x[(((b*h)+m)*w)+n]);
            }
         }
         y[(a*oh+i)*ow+j] = (y[(a*oh+i)*ow+j] + conv);
      }
   }
}

/**
 *  @brief Computes the 2d convolution over all feature maps.
 *
 *  For each output feature map, loops over input feature maps, computes the
 *  convolutions and sums them as per the definition of 2d convolution of a 4d
 *  weight tensor by a 2d input tensor.
 *
 *  @param *W
 *      a pointer to the base of the 4d weight tensor. Weights are organized
 *      in a W[a,b,i,j] fashion, where a is the index of the output feature
 *      map, b is the index of the input feature map, and (i,j) are the
 *      coordinates of a pixel in the filter. Data layout on memory is
 *      row-major. The __restrict__ keyword is used to indicate the compiler
 *      that W, x, y do not overlap.
 *  @param *x
 *      a pointer to the base of the input 3d tensor (a collection of feature
 *      maps). The inputs are organized in a x[b,i,j] fashion, where b is the
 *      index of the input feature map, and (i,j) are the coordinates of a
 *      pixel in the input feature map. The __restrict__ keyword is used to
 *      indicate the compiler that W, x, y do not overlap.
 *  @param *y
 *      a pointer to the base of the output 3d tensor (a collection of feature
 *      maps. The outputs are organized in a y[a,i,j] fashion, where a is the
 *      index of the output feature map, and (i,j) are the coordinates of a
 *      pixel in the output feature map. The __restrict__ keyword is used to
 *      indicate the compiler that W, x, y do not overlap.
 *  @param h
 *      the height of the input feature maps.
 *  @param w
 *      the width of the input feature maps.
 *  @param fs
 *      the size of the filters.
 *  @param a
 *      the index of output feature maps.
 *  @param nif
 *      the number of input feature maps.
 *  @param parallel_type
 *      the type of parallelization.
 */

#ifdef CCN_HWCE_ACCEL
void linalg_2dconv_hwce(
   data_t *__restrict__ W,
   data_t *__restrict__ x,
   data_t *__restrict__ y,
   int h,
   int w,
   int fs,
   int a,
   int nif,
   int parallel_type,
   unsigned qf
) {
   int oh = h-fs+1;
   int ow = w-fs+1;
   int id = -1;
   int b=0;
   int16_t *y_ptr = y + a*oh*ow;
   int16_t *x_base = x + b*h*w;
   int16_t *W_base;
   switch(fs) {
      case 3:
         W_base = W + a*nif*28 + b*28;
         id = conv16_hwce_3x3_async(W_base, x_base, y_ptr, h, w, nif, 0, qf);
         break;
      case 5:
         W_base = W + a*nif*28 + b*28;
         id = conv16_hwce_5x5_async(W_base, x_base, y_ptr, h, w, nif, 0, qf);
         break;
      case 7:
         W_base = W + a*nif*4*28 + b*4*28;
         id = conv16_hwce_7x7_async(W_base, x_base, y_ptr, h, w, nif, 0, qf);
         break;
      case 11:
         W_base = W + a*nif*9*28 + b*9*28;
         id = conv16_hwce_11x11_async(W_base, x_base, y_ptr, h, w, nif, 0, qf);
         break;
      default:
         break;
   }
   hwce_wait(id);
}
#endif /* CCN_HWCE_ACCEL */

void linalg_2dconv(
   data_t *__restrict__ W,
   data_t *__restrict__ x,
   data_t *__restrict__ y,
   int h,
   int w,
   int fs,
   int a,
   int nif,
   int parallel_type,
   unsigned qf
) {
   int oh = h-fs+1;
   int ow = w-fs+1;
   int b,ri;
   // #pragma omp parallel
   {
   if(parallel_type==PARALLEL_FEAT) {
      // #pragma omp for nowait
      for (b=0; b<nif; b++) {
         int16_t *y_ptr = y + a*oh*ow;
         int16_t *x_base = x + b*h*w + (fs-1)*w + (fs-1);
#if PULP_CHIP == CHIP_MIA || PULP_CHIP == CHIP_PULP3 || PULP_CHIP == CHIP_FULMINE || PULP_CHIP == CHIP_HONEY
         int16_t *W_base = W + a*nif*MULTIPLE4(fs*fs) + b*MULTIPLE4(fs*fs);
#else /* ~CHIP_MIA && ~CHIP_PULP3 */
         int16_t *W_base = W + a*nif*fs*fs + b*fs*fs;
#endif /* ~CHIP_MIA && ~CHIP_PULP3 */
#ifdef CONV_APPROX
         conv16_approx(W_base, x_base, y_ptr, w, oh, ow, fs, parallel_type);
#else
         conv16_unrolled_ptr(W_base, x_base, y_ptr, w, oh, ow, fs, parallel_type, qf);
#endif
      }
   }
   else {
      // #pragma omp parallel
      for (b=0; b<nif; b++) {
         int16_t *y_ptr = y + a*oh*ow;
         int16_t *x_base = x + b*h*w + (fs-1)*w + (fs-1);
#if PULP_CHIP == CHIP_MIA || PULP_CHIP == CHIP_PULP3 || PULP_CHIP == CHIP_FULMINE || PULP_CHIP == CHIP_HONEY
         int16_t *W_base = W + a*nif*MULTIPLE4(fs*fs) + b*MULTIPLE4(fs*fs);
#else /* ~CHIP_MIA && ~CHIP_PULP3 */
         int16_t *W_base = W + a*nif*fs*fs + b*fs*fs;
#endif /* ~CHIP_MIA && ~CHIP_PULP3 */
#ifdef CONV_APPROX
         conv16_approx(W_base, x_base, y_ptr, w, oh, ow, fs, parallel_type);
#else
         conv16_unrolled_ptr(W_base, x_base, y_ptr, w, oh, ow, fs, parallel_type, qf);
#endif
      }
   }
   }
}

/*
    void linalg_mvprod:
        computes the matrix by vector product. Unused because eigen_mvprod is
        way more performant.

    params:
    - data_t *A:
        a pointer to the base of the 2d weight matrix. Weights are organized in
        a A[b,i,j,a,oi,oj] fashion, where b is the index of the input feature
        map, (i,j) are the coordinates of a pixel in the input feature map, a is
        the index of the output feature map and (oi,oj) are the coordinates of a
        pixel in the output feature map. This 6d tensor is treated as a
        A[b_i_j,a_oi_oj] 2d matrix for the purpose of this calculation.
        Data layout for the matrix is *column-major* (due to a limit in
        PyConvNet), therefore from a C row-major perspective the real tensor
        layout is A[a,oi,oj,b,i,j]. The __restrict__ keyword is used to indicate
        the compiler that A, x, y do not overlap.
    - data_t *x:
        a pointer to the base of the input 3d tensor (a collection of feature
        maps). The inputs are organized in a x[b,i,j] fashion, where b is the
        index of the input feature map, and (i,j) are the coordinates of a pixel
        in the input feature map. The __restrict__ keyword is used to indicate
        the compiler that A, x, y do not overlap.
    - data_t *y:
        a pointer to the base of the output 3d tensor (a collection of feature
        maps). The outputs are organized in a y[a,oi,oj] fashion, where a is the
        index of the output feature map, and (oi,oj) are the coordinates of a
        pixel in the output feature map. The __restrict__ keyword is used to
        indicate the compiler that A, x, y do not overlap.
    - int M:
        the number of rows in the A matrix, i.e. the product of the number of
        input feature maps, the height of an input feature map and its width.
    - int N:
        the number of columns in the A matrix, i.e. the product of the number of
        output feature maps, the height of an output feature map and its width.
*/
void linalg_mvprod(data_t *__restrict__ A, data_t *__restrict b, data_t *__restrict__ x, data_t *__restrict__ y, int M, int N, unsigned qf) {
    #pragma omp parallel for
    for(int n=0; n<N; n++) {
        int accum = y[n] << qf;
        for(int m=0; m<M; m++) {
            accum += A[m*N+n] * x[m];
        }
#ifndef DONT_SATURATE
        // SAT-
        accum >>= qf;
        if(accum > +32767) {
           accum = 0x00007fff;
        }
        // SAT+
        else if(accum < -32768) {
           accum = 0xffff8000;
        }
#endif /* DONT_SATURATE */
        y[n] = accum;
    }
      // data_t *yp = malloc(sizeof(data_t)*N);
      // memset(yp, 0, sizeof(data_t)*N);
      // plp_matmul_i16_norm(A, x, yp, N, M, 1, PLPLIB_NORM_SHIFT(qf));
      // #pragma omp parallel for
      // for(int n=0; n<N; n++) {
      //     y[n] += yp[n];
      // }
      // free(yp);

}
