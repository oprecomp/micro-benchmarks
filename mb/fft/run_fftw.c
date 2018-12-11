/*
Example taken from Cineca training online.
Modified to use Quantum Espresso FFT package (FFTW2)
A.Emerson 10-Apr-2017
*/

# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include "fftw.h"
# include "oprecomp.h"

#define fftw_complex FFTW_COMPLEX

int main ( int argc, char*argv[])

{
  int i;
  //const int n = 67108864;

  int n;
  fftw_complex *in;
  fftw_complex *out;
  fftw_complex *newout;
  fftw_plan plan_backward;
  fftw_plan plan_forward;
  double cdiff, prec, diffr, diffi;

  if (argc !=2) {
     printf("Array length needed\n");
     exit(1);
  }
  n=atoi(argv[1]);
  printf("# array_length: %d\n",n);

/* Create arrays. */
   in = fftw_malloc ( sizeof ( fftw_complex ) * n );
   out = fftw_malloc ( sizeof ( fftw_complex ) * n );
   newout = fftw_malloc ( sizeof ( fftw_complex ) * n );
/* Initialize data */
   for ( i = 0; i < n; i++ )
   { 
      if (i >= (n/4-1) && (3*n/4-1)) 
        {
          in[i].re = 1.;
          in[i].im = 0.;
        } 
      else
        {
          in[i].re = 0.;
          in[i].im = 0.;
        }       
   }

// oprecomp timing
   oprecomp_start();
   do {

/* Create plans. */
   plan_forward = fftw_create_plan ( n, FFTW_FORWARD, FFTW_ESTIMATE );
   plan_backward = fftw_create_plan ( n, FFTW_BACKWARD, FFTW_ESTIMATE );

/* Compute transform (as many times as desired) */
   fftw ( plan_forward, 1, in, 1,0, out, 1,0); 

/* Normalization */
   for ( i = 0; i < n; i++ ) { 
   out[i].re = out[i].re/n;
   out[i].im = out[i].im/n;
   }
/* Compute anti-transform */
   fftw ( plan_backward, 1,out,1,0,newout,1,0);

// oprecomp timing
   } while (oprecomp_iterate() );
   oprecomp_stop();

/* Check results */
   prec = 1.0e-7;
   cdiff = 0.;
   for( i = 0; i < n; i++ ) {
      diffr = fabs(in[i].re - newout[i].re);
      diffi = fabs(in[i].im - newout[i].im);
      if ( cdiff < diffr) {
         cdiff = diffr;
      }
      if( cdiff < diffi) {
         cdiff = diffi;
      }
   }
   printf("max diff =%g\n",cdiff);
   if(cdiff > prec)
      printf("Results are incorrect\n");
   else 
      printf("Results are correct\n");

/* Print results */
//   for ( i = 0; i < n; i++ )
//   { 
//    printf("i = %d, in = (%f), out = (%f,%f), newout = (%f)\n",i,in[i].re,out[i].re,out[i].im,newout[i].re);
//   }
/* deallocate and destroy plans */
   fftw_destroy_plan ( plan_forward );
   fftw_destroy_plan ( plan_backward );
   fftw_free ( in );
   fftw_free ( newout );
   fftw_free ( out );

  return 0;
}

