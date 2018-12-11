/*
	StencilProbe Heat Equation
	Implements 7pt stencil from Chombo's heattut example.
*/
#include <stdio.h>
#include <math.h>
#include "common.h"

double diffnorm(double* A0, double* Anext, int nx, int ny, int nz);


#ifdef STENCILTEST
void StencilProbe_naive(double* A0, double* Anext, int nx, int ny, int nz,
			int tx, int ty, int tz, int timesteps) {
#else
void StencilProbe(double *A0, double *Anext, int nx, int ny, int nz,
                  int tx, int ty, int tz, int timesteps) {
#endif
  // Fool compiler so it doesn't insert a constant here
  double fac = A0[0];
  double r16=(1.0/6.0);
  double *temp_ptr;
  int i, j, k, t;
  double rnorm,norm; 

  norm=0.0;
  for (t = 0; t < timesteps; t++) {
    #pragma omp parallel for
    for (k = 1; k < nz - 1; k++) {
      for (j = 1; j < ny - 1; j++) {
	for (i = 1; i < nx - 1; i++) {
	  Anext[Index3D (nx, ny, i, j, k)] = 
	    r16*(A0[Index3D (nx, ny, i, j, k + 1)] +
	    A0[Index3D (nx, ny, i, j, k - 1)] +
	    A0[Index3D (nx, ny, i, j + 1, k)] +
	    A0[Index3D (nx, ny, i, j - 1, k)] +
	    A0[Index3D (nx, ny, i + 1, j, k)] +
	    A0[Index3D (nx, ny, i - 1, j, k)])
             - A0[Index3D (nx, ny, i, j, k)]/(fac*fac);
	}
      }
    }
    temp_ptr = A0;
    A0 = Anext;
    Anext = temp_ptr;
    norm=diffnorm(Anext, A0, nx, ny, nz);
        printf("Norm=%g \n",norm);

  }
}
