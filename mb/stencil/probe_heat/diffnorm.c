#include <math.h>
#include "common.h"

double diffnorm(double*A, double*B, int nx, int ny, int nz)
{
  int i,j,k;
double diff,norm;

norm=0.0;
  for (k = 1; k < nz - 1; k++) 
    for (j = 1; j < ny - 1; j++) 
      for (i = 1; i < nx - 1; i++) { 
	diff= B[Index3D(nx,ny,i,j,k)]-A[Index3D(nx,ny,i,j,k)] ;
        norm += diff*diff;
      }
  norm=sqrt(norm);
  return norm;
}

