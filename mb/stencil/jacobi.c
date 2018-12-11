#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "oprecomp.h"
#include <omp.h>

// Grid boundary conditions
#define RIGHT 1.0
#define LEFT 1.0
#define TOP 1.0
#define BOTTOM 10.0

// precision
#ifdef SINGLE
  typedef float REAL;
#define TOLERANCE 0.0001f
#else
  typedef double REAL;
#define TOLERANCE 0.0001
#endif


// Algorithm settings
#define NPRINT 1000
#define MAX_ITER 100000


int main(int argc, char*argv[]) {

  int k;
  REAL tmpnorm,bnorm,norm;

  if (argc !=4) {
    printf("usage: $argv[0] GRIDX GRIDY num_threads\n");
      return(1);
  }
#ifdef SINGLE
  printf("Using single precision\n");
#else
  printf("Using double precision \n");
#endif


  int nx=atoi(argv[1]);
  int ny=atoi(argv[2]);
  int ny2=ny+2;
  int nthds=atoi(argv[3]);

  printf("grid size %d X %d \n",ny,ny);
  REAL *grid= (REAL*)malloc(sizeof(REAL)*(nx+2)*(ny+2));
  REAL *grid_new= (REAL*)malloc(sizeof(REAL)*(nx+2)*(ny+2));
  REAL *temp= (REAL*)malloc(sizeof(REAL)*(nx+2)*(ny+2));

// omp threads
//
  printf("# num_threads:%d\n",nthds);

  // Initialise Grid boundaries
  int i,j;
  for (i=0;i<ny+2;i++) {
    grid_new[i]=grid[i]=TOP;
    j=(ny+2)*(nx+1)+i;
    grid_new[j]=grid[j]=BOTTOM;
  }
  for (i=1;i<nx+1;i++) {
    j=(ny+2)*i;
    grid_new[j]=grid[j]=LEFT;
    grid_new[j+ny+1]=grid[j+ny+1]=RIGHT;
  }
   
  // Initialise rest of grid
  for (i=1;i<=nx;i++) 
    for (j=1;j<=ny;j++)
      k=(ny+2)*i+j;
      grid_new[k]=grid[k]=0.0;
   
/*  for (i=0;i<=nx+1;i++) {
    for (j=0;j<=ny+1;j++){
      printf("->%lf ",grid[j+i*(ny+2)]);
    }
    printf("\n");
  }
*/

  tmpnorm=0.0;
  for (i=1;i<=nx;i++) {
    for (j=1;j<=ny;j++) {
      k=(ny+2)*i+j;            
      tmpnorm=tmpnorm+(REAL)pow(grid[k]*4.0-grid[k-1]-grid[k+1] - grid[k-(ny+2)] - grid[k+(ny+2)], 2); 

    }
  }
  bnorm=sqrt(tmpnorm);

//  start oprecomp timing **
  oprecomp_start();

//    MAIN LOOP 
  int iter;
  for (iter=0; iter<MAX_ITER; iter++) {


    tmpnorm=0.0;

#pragma omp parallel for num_threads(nthds) collapse(2) default(shared) private (i,j,k) reduction(+:tmpnorm) 
    for (i=1;i<=nx;i++) {
     for (j=1;j<=ny;j++) {
      k=(ny+2)*i+j;
      tmpnorm=tmpnorm+(REAL)pow(grid[k]*4.0-grid[k-1]-grid[k+1] - grid[k-(ny+2)] - grid[k+(ny+2)], 2); 
    }
  }
   
    norm=(REAL)sqrt(tmpnorm)/bnorm;

    if (norm < TOLERANCE) break;
    
#pragma omp parallel for num_threads(nthds) collapse(2) default(shared) private(i,j,k)

    for (i=1;i<=nx;i++) {
      for (j=1;j<=ny;j++) {
        k=(ny+2)*i+j;    
	grid_new[k]=0.25 * (grid[k-1]+grid[k+1] + grid[k-(ny+2)] + grid[k+(ny+2)]);
      }
    }

  
    memcpy(temp, grid_new, sizeof(REAL) * (nx + 2) * (ny+2));
    memcpy(grid_new, grid, sizeof(REAL) * (nx + 2) * (ny+2));
    memcpy(grid, temp, sizeof(REAL) * (nx + 2) * (ny+2));

    if (iter % NPRINT ==0) printf("Iteration =%d ,Relative norm=%e\n",iter,norm);
  }

  printf("Terminated on %d iterations, Relative Norm=%e \n", iter,norm);
  
//  for (i=0;i<=nx+1;i++) {
//    for (j=0;j<=ny+1;j++){
//     printf("->%lf ",grid[j+i*(ny+2)]);
//    }
//    printf("\n");
//  }

// stop oprecomp timing **
   oprecomp_stop();


  free(grid);
  free(temp);
  free(grid_new);



  return 0;
    

  }
