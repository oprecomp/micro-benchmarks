#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "oprecomp.h"

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
#define MAX_ITER 200000


__global__
void stencil_sum(REAL*grid, REAL *grid_new, int nx, int ny)
{
  int index=blockIdx.x * blockDim.x +threadIdx.x; // global thread id

  int nrow=index/ny;
  int diff=index-(nrow*ny);
  int k=(nrow+1)*(ny+2)+diff+1;

  if (index<nx*ny) 
      grid_new[k]=0.25 * (grid[k-1]+grid[k+1] + grid[k-(ny+2)] + grid[k+(ny+2)]);
}

__global__
void stencil_norm(REAL*grid, REAL*arraynorm, int nx, int ny)
{
  int index=blockIdx.x * blockDim.x +threadIdx.x; // globEl thread id
  
  int nrow=index/ny;
  int diff=index-(nrow*ny);
  int k=(nrow+1)*(ny+2)+diff+1;

  if (index<nx*ny)
     arraynorm[index]=(REAL)pow(grid[k]*4.0-grid[k-1]-grid[k+1] - grid[k-(ny+2)] - grid[k+(ny+2)], 2);

}

//   
//  Taken from CUDA document. Uses  Reduce v4. 
//  Partial sums performed for each block
//  

__global__
void reduce(REAL* g_idata, REAL *g_odata, int nx, int ny) {
extern __shared__ REAL sdata[];

  int tid=threadIdx.x;
  int i=blockIdx.x*(blockDim.x*2) + threadIdx.x;

  if ( (i+blockDim.x) < (nx*ny) ) 
     sdata[tid]=g_idata[i]+g_idata[i+blockDim.x];
  else
     sdata[tid]=0.0;

  __syncthreads();

  for(int s=blockDim.x/2;s>0;s>>=1) {
     if (tid<s) {
        sdata[tid] += sdata[tid+s];
     }
     __syncthreads();
  }
  if (tid ==0) { 
      g_odata[blockIdx.x]=sdata[0];
  }
}

void getDeviceInfo() {

  int nDevices;
  cudaGetDeviceCount(&nDevices);
  for (int i = 0; i < nDevices; i++) {
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, i);
    printf("Device Number: %d\n", i);
    printf("  Device name: %s\n", prop.name);
    printf("  Memory Clock Rate (KHz): %d\n",
           prop.memoryClockRate);
    printf("  Memory Bus Width (bits): %d\n",
           prop.memoryBusWidth);
    printf("  Peak Memory Bandwidth (GB/s): %f\n\n",
           2.0*prop.memoryClockRate*(prop.memoryBusWidth/8)/1.0e6);
  }

}


// MAIN LOOP 
int main(int argc, char*argv[]) {

  int k;
  REAL tmpnorm,bnorm,norm;

  printf("Jacobi 4-point stencil\n");
  printf("----------------------\n\n");

  if (argc !=3) {
    printf("usage: $argv[0] GRIDX GRIDY \n");
      return(1);
  }

  // GPU info
  getDeviceInfo();

 // One device
  cudaSetDevice(0);

#ifdef SINGLE
  printf("Using single precision\n");
#else
  printf("Using double precision \n");
#endif
  printf("sizeof(REAL)=%d\n",sizeof(REAL));

  int nx=atoi(argv[1]);
  int ny=atoi(argv[2]);

  printf("grid size %d X %d \n",ny,ny);

// GPU threads/block

  int blockSize=256;
  int numBlocks = ((nx*ny)+blockSize-1)/blockSize;
  printf("numBlocks=%d\n",numBlocks);

//
// host allocated memory
//

  REAL *grid= (REAL*)malloc(sizeof(REAL)*(nx+2)*(ny+2));
  REAL *grid_new= (REAL*)malloc(sizeof(REAL)*(nx+2)*(ny+2));
  REAL *arraynorm= (REAL*)malloc(sizeof(REAL)*nx*ny);
  REAL*blocknorm=(REAL*)malloc(sizeof(REAL)*numBlocks);

  //
  // Device allocated memory
  //

  REAL *d_grid, *d_grid_new, *d_arraynorm, *d_blocknorm;
  cudaMalloc(&d_grid,(nx+2)*(ny+2)*sizeof(REAL));
  cudaMalloc(&d_grid_new,(nx+2)*(ny+2)*sizeof(REAL));
  cudaMalloc(&d_arraynorm,nx*ny*sizeof(REAL));
  cudaMalloc(&d_blocknorm,numBlocks*sizeof(REAL)); 

// shared memory size on GPU 
  int smemsize=blockSize*sizeof(REAL);

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
   
  // initial norm factor
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

// copy arrays to device

  cudaMemcpy(d_grid,grid,(nx+2)*(ny+2)*sizeof(REAL), cudaMemcpyHostToDevice);
  cudaMemcpy(d_grid_new,grid_new,(nx+2)*(ny+2)*sizeof(REAL), cudaMemcpyHostToDevice);


//    MAIN LOOP 
  int iter;
  for (iter=0; iter<MAX_ITER; iter++) {

    // calculate norm array
    stencil_norm<<<numBlocks,blockSize>>>(d_grid,d_arraynorm,nx,ny); 
    
    // perform reduction
    reduce<<<numBlocks,blockSize,smemsize>>>(d_arraynorm,d_blocknorm,nx,ny);
    cudaMemcpy(blocknorm,d_blocknorm,numBlocks*sizeof(REAL),cudaMemcpyDeviceToHost);
 
    // sum up temporary block sums
    tmpnorm=0.0;
    for (i=0;i<numBlocks;i++) {
      tmpnorm=tmpnorm+blocknorm[i];
    }
   
    norm=(REAL)sqrt(tmpnorm)/bnorm;

    if (norm < TOLERANCE) break;

    stencil_sum<<<numBlocks,blockSize>>>(d_grid,d_grid_new,nx,ny);

  // Wait for GPU to finish
  cudaDeviceSynchronize();

    REAL *temp=d_grid_new;
    d_grid_new=d_grid;
    d_grid=temp;

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

  cudaFree(d_grid);
  cudaFree(d_grid_new);
  cudaFree(d_arraynorm);

  free(grid);
  free(grid_new);
  free(arraynorm);

  return 0;
    

  }
