#include <cmath>
#include <cassert>
#include <chrono>
#include "kernels.h"

#include "IO.hpp"
#include "Show.hpp"

/////////////////////////////////////////////////////////////////////////////
// GPU CODE 
/////////////////////////////////////////////////////////////////////////////

// texture<RefNumberType> texVec;
// texture<RefNumberType> texA;


inline unsigned PowerOfTwoAlign( unsigned int n )
{
	int PowerOfTwoAlign  = 1;
	while( PowerOfTwoAlign < n ) PowerOfTwoAlign = PowerOfTwoAlign << 1;
	return PowerOfTwoAlign;
} //*/


// KERNEL WITH BLOCKS. OK
__global__ void GPU_DENSE_PR(	RefNumberType* const dataA,
								const RefNumberType* const vecOld,
								RefNumberType* const vec,
								const double d )
{
	// row loop runs with blockIdx.x 
	const double InvFactor 	= 1/((double) gridDim.x );

	double sum = 0;
	for( size_t j = 0; j<gridDim.x; ++j )
	{
		// sum += A[row][j]*pold[j];
		sum += dataA[blockIdx.x*gridDim.x + j]*vecOld[j];
	}
	// sum now contains the scalar product A[row,:] DOT p
	vec[blockIdx.x] 	= d*sum + (1-d)*InvFactor;
}

// KERNEL WITH THREADS OK AND FASTER
__global__ void GPU_DENSE_PR_2(	RefNumberType* const dataA,
								const RefNumberType* const vecOld,
								RefNumberType* const vec,
								const double d, 
								int n )
{
	// loop runs with threads
	int tid = threadIdx.x + blockIdx.x*blockDim.x;
	const double InvFactor 	= 1/((double) n );

	if( tid < n )
	{

		double sum = 0;
		for( size_t j = 0; j<n; ++j )
		{
			// sum += A[row][j]*pold[j];
			sum += dataA[tid*n + j]*vecOld[j];
		}
		// sum now contains the scalar product A[row,:] DOT p
		vec[tid] 	= d*sum + (1-d)*InvFactor;
	}
} 

// KERNEL WITH THREADS + TEXTURE MEMORY
__global__ void GPU_DENSE_PR_3(	RefNumberType* const dataA,
								// const RefNumberType* const vecOld,
								RefNumberType* const vec,
								const double d,
								int n  )
{
	// row loop runs with blockIdx.x 
	int tid = threadIdx.x + blockIdx.x*blockDim.x;	
	const double InvFactor 	= 1/((double) n );
	
	if( tid < n )
	{
		double sum = 0;
		for( size_t j = 0; j<n; ++j )
		{
			// sum += A[row][j]*pold[j];
			// sum += dataA[threadIdx.x*blockDim.x + j]*vecOld[j];
			// sum += dataA[tid*n + j]*tex1Dfetch(texVec, (int) j);
			// sum += tex1Dfetch(texA, tid*n + j)*tex1Dfetch(texVec, j);
		}
		// sum now contains the scalar product A[row,:] DOT p
		vec[tid] 	= d*sum + (1-d)*InvFactor;
	}
} 

// KERNEL WITH THREADS
__global__ void GPU_CSR_PR(		const RefNumberType* const data,
								const size_t* const row_ptr,
								const size_t* const col_idx,
								const RefNumberType* const vecOld,
								RefNumberType* const vec,
								const double d,
								int n )
{
	// row loop runs with blockIdx.x 
	int tid = threadIdx.x + blockIdx.x*blockDim.x;	
	const double InvFactor 	= 1/((double) n );
	
	if( tid < n )
	{
		double sum = 0;

		for( size_t idx = row_ptr[tid]; idx < row_ptr[tid+1]; ++idx )
		{
			sum += data[idx]*vecOld[ col_idx[idx] ];
		}
		// sum now contains the scalar product A[row,:] DOT p
		vec[tid] 	= d*sum + (1-d)*InvFactor;
	}
} 

// KERNEL WITH THREADS
__global__ void GPU_CSR_PR_PartialSum(		const RefNumberType* const data,
											const size_t* const row_ptr,
											const size_t* const col_idx,
											const RefNumberType* const vecOld,
											const RefNumberType* const partialSum,
											RefNumberType* const vec,
											const double d,
											int n )
{
	// row loop runs with blockIdx.x 
	int tid = threadIdx.x + blockIdx.x*blockDim.x;	
	const double InvFactor 	= 1/((double) n );
	
	if( tid < n )
	{
		double sum = (*partialSum);

		for( size_t idx = row_ptr[tid]; idx < row_ptr[tid+1]; ++idx )
		{
			sum += data[idx]*vecOld[ col_idx[idx] ];
		}
		// sum now contains the scalar product A[row,:] DOT p
		vec[tid] 	= d*sum + (1-d)*InvFactor;
	}
} 

__global__ void GPU_Reduce_PartialSum( 	const RefNumberType* const vecOld,
									const size_t* const MaskLine,
									size_t MaskLine_size,
									RefNumberType* const result,
									RefNumberType defaultValue )
{
	(*result) = 0;
	for( size_t i = 0; i<MaskLine_size; ++i)
	{
		(*result) += vecOld[ MaskLine[i] ];
	}
	(*result) *= defaultValue;
}

__global__ void GPU_Reduce_Error(	RefNumberType* const tmpErr,
									RefNumberType* const vecOld,
									const RefNumberType* const vec, int n )
{
	(*tmpErr) = 0;
	for( size_t row = 0; row < n; ++row)
	{
		// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
		(*tmpErr) 	   += (vec[row]-vecOld[row])*(vec[row]-vecOld[row]);
	}
}

const int THREADS_PER_BLOCK = 1024;
static_assert( THREADS_PER_BLOCK == 1024 , "Power of two value required!");
// int BlocksPerGrid = (n+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;

__global__ void GPU_Reduce_Error_2(	RefNumberType* const partial_tmpErr,
									const RefNumberType* const vecOld,
									const RefNumberType* const vec, int n )
{
	//assert( THREADS_PER_BLOCK == PowerOfTwoAlign( THREADS_PER_BLOCK ) );
	__shared__ RefNumberType cache[THREADS_PER_BLOCK];

	int tid = threadIdx.x + blockIdx.x * blockDim.x;

	float tmp = 0;
	while( tid < n )
	{
		tmp += (vec[tid]-vecOld[tid])*(vec[tid]-vecOld[tid]);
		tid += blockDim.x * gridDim.x;
	}

	cache[threadIdx.x] = tmp;

	__syncthreads();

	int i = blockDim.x / 2;

	while( i!= 0)
	{
		if( threadIdx.x  < i  )
		{
			cache[threadIdx.x] += cache[threadIdx.x + i];
		}
		__syncthreads();
		i/=2;
	}

	if( threadIdx.x == 0)
	{
		//(*tmpErr) = cache[0];
		partial_tmpErr[ blockIdx.x ] = cache[0];
	}
}


void showVec( RefNumberType* devPtr, int n )
{ 
	RefNumberType* tmpData = new RefNumberType[n];
	cudaMemcpy( tmpData, devPtr, n*sizeof(RefNumberType), cudaMemcpyDeviceToHost );
	for( int i = 0; i < n; ++ i )
	{
		printf("\t%.10e", (RefNumberType) tmpData[i]);
	}
	delete[] tmpData;
	printf("\n\n");
}

template<typename T> 
void showVecT( T* devPtr, int n )
{ 
	T* tmpData = new T[n];
	cudaMemcpy( tmpData, devPtr, n*sizeof(T), cudaMemcpyDeviceToHost );
	for( int i = 0; i < n; ++ i )
	{
		printf("\t%.10e", (T) tmpData[i]);
	}
	delete[] tmpData;
	printf("\n\n");
}

template<typename T> 
void showVecTu( T* devPtr, int n )
{ 
	T* tmpData = new T[n];
	cudaMemcpy( tmpData, devPtr, n*sizeof(T), cudaMemcpyDeviceToHost );
	for( int i = 0; i < n; ++ i )
	{
		printf("\t%u", (T) tmpData[i]);
	}
	delete[] tmpData;
	printf("\n\n");
}

std::vector<RefNumberType> GPU_PageRank_Dense( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps )
{	
	printf("GPU - CODE, n = %u \n", A.size() );

	//squared matrix required (n x n)
	assert( A.size() == A[0].size() );

	unsigned k 			= 0;
	size_t n 			= A.size();
	double InvFactor 	= 1/((double) n);
	RefNumberType tmpErr = 2*eps;
	int BlocksPerGrid = (n+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;

	//------------------------------------------------------------------
	// GPU MEMORY
	//------------------------------------------------------------------
	RefNumberType *dev_dataA;
	cudaMalloc( (void**) &dev_dataA, n*n*sizeof(RefNumberType) );

	RefNumberType *dev_vec;
	cudaMalloc( (void**) &dev_vec, n*sizeof(RefNumberType) );

	RefNumberType *dev_vecOld;
	cudaMalloc( (void**) &dev_vecOld, n*sizeof(RefNumberType) );

	RefNumberType *dev_partial_tmpErr;
	cudaMalloc( (void**) &dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType) );

	// TEXTURE - MEMORY
	// cudaBindTexture( NULL, texVec, dev_vecOld, n*sizeof(RefNumberType) );
	// cudaBindTexture( NULL, texA, dev_dataA, n*n*sizeof(RefNumberType) );

	// cudaEvent_t start, stop; 
	// cudaEventCreate( &start );
	// cudaEventCreate( &stop );


	//------------------------------------------------------------------
	// GPU UPLOAD
	//------------------------------------------------------------------
	RefNumberType* dataA = new RefNumberType[n*n];
	MatrixToFlatData( dataA, A );
	cudaMemcpy( dev_dataA, dataA, n*n*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] dataA;

	// uniform vecotor of length n, with values 1/n at all positions
	RefNumberType* init_vec_data = new RefNumberType[n];
	for( int i = 0; i < n; ++ i )
	{
		init_vec_data[i] = InvFactor;
	}
	cudaMemcpy( dev_vec, init_vec_data, n*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] init_vec_data;

	//float time_kernel = 0;
	//float time_kernel2 = 0;
	RefNumberType* partial_tmpErr = new RefNumberType[BlocksPerGrid];

	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(n^2): dense matrix-vector multiplication

		// printf("BEFORE\n");
		// showVec( dev_vecOld, n);
		// showVec( dev_vec, n);

		cudaMemcpy( dev_vecOld, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToDevice );

		// printf("after copy\n");
		// showVec( dev_vecOld, n);
		// showVec( dev_vec, n);
		// cudaEventRecord( start, 0 );

		// GPU_DENSE_PR<<<n,1>>>( dev_dataA, dev_vecOld, dev_vec, d );
		GPU_DENSE_PR_2<<< (n+31)/32, 32>>>( dev_dataA, dev_vecOld, dev_vec, d, n );
		// GPU_DENSE_PR_3<<< (n+31)/32, 32>>>( dev_dataA, /*dev_vecOld,*/ dev_vec, d, n );

		// cudaEventRecord( stop, 0 );
		// cudaEventSynchronize( stop );

		// float deltatime;
		// cudaEventElapsedTime( &deltatime, start, stop );
		// time_kernel += deltatime;

		// printf("AFTER\n");
		// showVec( dev_vecOld, n);
		// showVec( dev_vec, n);
		// cudaEventRecord( start, 0 );

		// printf("n = %i\n", n );
		// assert( n == THREADS_PER_BLOCK);
		// GPU_Reduce_Error<<<1,1>>>( dev_tmpErr, dev_vecOld, dev_vec, n);

		assert( THREADS_PER_BLOCK == PowerOfTwoAlign( THREADS_PER_BLOCK ) );
		GPU_Reduce_Error_2<<< BlocksPerGrid, THREADS_PER_BLOCK >>>( dev_partial_tmpErr, dev_vecOld, dev_vec, n);

		//------------------------------------------------------------------
		// GPU DOWNLOAD AND COMPLETE ERROR REDUCTION
		//------------------------------------------------------------------
		tmpErr = 0;
		cudaMemcpy( partial_tmpErr, dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType), cudaMemcpyDeviceToHost );

		for( unsigned i = 0; i<BlocksPerGrid; ++i)
		{
			tmpErr += partial_tmpErr[i];
		}

		// cudaEventRecord( stop, 0 );
		// cudaEventSynchronize( stop );
		// cudaEventElapsedTime( &deltatime, start, stop );
		// time_kernel2 += deltatime;

		tmpErr = sqrt( tmpErr );
		printf("[k = %u]: %e\n", k++, tmpErr );

		// if( k > 1000 ) break;
	}
	delete[] partial_tmpErr;

	//------------------------------------------------------------------
	// GPU DOWNLOAD
	//------------------------------------------------------------------
	RefNumberType* tmpData = new RefNumberType[n];
	cudaMemcpy( tmpData, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToHost );
	std::vector<RefNumberType> ret;
	ret.assign(tmpData, tmpData + n);
	delete[] tmpData;
	//------------------------------------------------------------------
	// GPU CLEANUP
	//------------------------------------------------------------------
	// cudaEventDestroy( start );
	// cudaEventDestroy( stop );

	// cudaUnbindTexture( texVec );
	// cudaUnbindTexture( texA );

	cudaFree( dev_dataA );
	cudaFree( dev_vec );
	cudaFree( dev_vecOld );
	cudaFree( dev_partial_tmpErr );

	return ret;
}


std::vector<RefNumberType> GPU_PageRank_CSR( CSRType<RefNumberType> const &S, size_t n, RefNumberType d, RefNumberType eps )
{	
	printf("GPU - CODE, nZZ = %u \n", S.data.size() );

	unsigned k 			 = 0;
	double InvFactor 	 = 1/((double) n);
	RefNumberType tmpErr = 2*eps;
	int BlocksPerGrid = (n+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;

	//------------------------------------------------------------------
	// GPU MEMORY
	//------------------------------------------------------------------
	RefNumberType *dev_data;
	cudaMalloc( (void**) &dev_data, S.data.size()*sizeof(RefNumberType) );

	size_t *dev_row_ptr;
	cudaMalloc( (void**) &dev_row_ptr, S.row_ptr.size()*sizeof(size_t) );

	size_t *dev_col_idx;
	cudaMalloc( (void**) &dev_col_idx, S.col_idx.size()*sizeof(size_t) );

	RefNumberType *dev_vec;
	cudaMalloc( (void**) &dev_vec, n*sizeof(RefNumberType) );

	RefNumberType *dev_vecOld;
	cudaMalloc( (void**) &dev_vecOld, n*sizeof(RefNumberType) );

	RefNumberType *dev_partial_tmpErr;
	cudaMalloc( (void**) &dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType) );

	// TEXTURE - MEMORY
	// cudaBindTexture( NULL, texVec, dev_vecOld, n*sizeof(RefNumberType) );
	// cudaBindTexture( NULL, texA, dev_dataA, n*n*sizeof(RefNumberType) );

	//------------------------------------------------------------------
	// GPU UPLOAD
	//------------------------------------------------------------------
	RefNumberType* tmp1 = new RefNumberType[ S.data.size() ];
	std::copy( S.data.begin(), S.data.end(), tmp1 );
	cudaMemcpy( dev_data, tmp1, S.data.size()*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] tmp1;

	size_t* tmp2 = new size_t[ S.row_ptr.size() ];
	std::copy( S.row_ptr.begin(), S.row_ptr.end(), tmp2 );
	cudaMemcpy( dev_row_ptr, tmp2, S.row_ptr.size()*sizeof(size_t), cudaMemcpyHostToDevice );
	delete[] tmp2;

	size_t* tmp3 = new size_t[ S.col_idx.size() ];
	std::copy( S.col_idx.begin(), S.col_idx.end(), tmp3 );
	cudaMemcpy( dev_col_idx, tmp3, S.col_idx.size()*sizeof(size_t), cudaMemcpyHostToDevice );
	delete[] tmp3;

	RefNumberType* init_vec_data = new RefNumberType[n];
	for( int i = 0; i < n; ++ i )
	{
		init_vec_data[i] = InvFactor;
	}
	cudaMemcpy( dev_vec, init_vec_data, n*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] init_vec_data;

	// float time_kernel = 0;
	// float time_kernel2 = 0;
	RefNumberType* partial_tmpErr = new RefNumberType[BlocksPerGrid];

	while( tmpErr > eps )
	{

		// copy current state to old vector.
		cudaMemcpy( dev_vecOld, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToDevice );

		// printf("after copy\n");
		// showVec( dev_vecOld, n);
		// showVec( dev_vec, n);

		// cudaEventRecord( start, 0 );

		GPU_CSR_PR<<< (n+31)/32, 32>>>( dev_data, dev_row_ptr, dev_col_idx, dev_vecOld, dev_vec, d, n );

		// cudaEventRecord( stop, 0 );
		// cudaEventSynchronize( stop );

		// float deltatime;
		// cudaEventElapsedTime( &deltatime, start, stop );
		// time_kernel += deltatime;

		// printf("AFTER\n");
		// showVec( dev_vecOld, n);
		// showVec( dev_vec, n);
		// cudaEventRecord( start, 0 );

		// printf("n = %i\n", n );
		// assert( n == THREADS_PER_BLOCK);
		// GPU_Reduce_Error<<<1,1>>>( dev_tmpErr, dev_vecOld, dev_vec, n);
		assert( THREADS_PER_BLOCK == PowerOfTwoAlign( THREADS_PER_BLOCK ) );
		GPU_Reduce_Error_2<<< BlocksPerGrid, THREADS_PER_BLOCK >>>( dev_partial_tmpErr, dev_vecOld, dev_vec, n);

		//------------------------------------------------------------------
		// GPU DOWNLOAD AND COMPLETE ERROR REDUCTION
		//------------------------------------------------------------------
		tmpErr = 0;
		cudaMemcpy( partial_tmpErr, dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType), cudaMemcpyDeviceToHost );

		for( unsigned i = 0; i<BlocksPerGrid; ++i)
		{
			tmpErr += partial_tmpErr[i];
		}
		// cudaEventRecord( stop, 0 );
		// cudaEventSynchronize( stop );
		// cudaEventElapsedTime( &deltatime, start, stop );
		// time_kernel2 += deltatime;

		tmpErr = sqrt( tmpErr );
		printf("[k = %u]: %e\n", k++, tmpErr );

		// if( k > 1000 ) break;
	}

	delete[] partial_tmpErr;

	//------------------------------------------------------------------
	// GPU DOWNLOAD
	//------------------------------------------------------------------
	RefNumberType* tmpData = new RefNumberType[n];
	cudaMemcpy( tmpData, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToHost );
	std::vector<RefNumberType> ret;
	ret.assign(tmpData, tmpData + n);
	delete[] tmpData;
	//------------------------------------------------------------------
	// GPU CLEANUP
	//------------------------------------------------------------------
	// cudaEventDestroy( start );
	// cudaEventDestroy( stop );

	//cudaUnbindTexture( texVec );
	//cudaUnbindTexture( texA );

	cudaFree( dev_data );
	cudaFree( dev_row_ptr );
	cudaFree( dev_col_idx );
	cudaFree( dev_vec );
	cudaFree( dev_vecOld );
	cudaFree( dev_partial_tmpErr );

	return ret;
}

std::vector<RefNumberType> GPU_PageRank_CSR_OPT( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps )
{
	printf("GPU - CODE, nZZ = %u \n", S.data.size() );

	unsigned k 			 = 0;
	double InvFactor 	 = 1/((double) n);
	RefNumberType tmpErr = 2*eps;
	int BlocksPerGrid = (n+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;

	//------------------------------------------------------------------
	// GPU MEMORY
	//------------------------------------------------------------------
	RefNumberType *dev_data;
	cudaMalloc( (void**) &dev_data, S.data.size()*sizeof(RefNumberType) );

	size_t *dev_row_ptr;
	cudaMalloc( (void**) &dev_row_ptr, S.row_ptr.size()*sizeof(size_t) );

	size_t *dev_col_idx;
	cudaMalloc( (void**) &dev_col_idx, S.col_idx.size()*sizeof(size_t) );

	size_t *dev_MaskLine;
	cudaMalloc( (void**) &dev_MaskLine, MaskLine.size()*sizeof(size_t) );

	RefNumberType *dev_vec;
	cudaMalloc( (void**) &dev_vec, n*sizeof(RefNumberType) );

	RefNumberType *dev_vecOld;
	cudaMalloc( (void**) &dev_vecOld, n*sizeof(RefNumberType) );

	RefNumberType *dev_partial_tmpErr;
	cudaMalloc( (void**) &dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType) );

	RefNumberType *dev_partialSum;
	cudaMalloc( (void**) &dev_partialSum, sizeof(RefNumberType) );
	// TEXTURE - MEMORY
	// cudaBindTexture( NULL, texVec, dev_vecOld, n*sizeof(RefNumberType) );
	// cudaBindTexture( NULL, texA, dev_dataA, n*n*sizeof(RefNumberType) );

	//------------------------------------------------------------------
	// GPU UPLOAD
	//------------------------------------------------------------------
	RefNumberType* tmp1 = new RefNumberType[ S.data.size() ];
	std::copy( S.data.begin(), S.data.end(), tmp1 );
	cudaMemcpy( dev_data, tmp1, S.data.size()*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] tmp1;

	size_t* tmp2 = new size_t[ S.row_ptr.size() ];
	std::copy( S.row_ptr.begin(), S.row_ptr.end(), tmp2 );
	cudaMemcpy( dev_row_ptr, tmp2, S.row_ptr.size()*sizeof(size_t), cudaMemcpyHostToDevice );
	delete[] tmp2;

	size_t* tmp3 = new size_t[ S.col_idx.size() ];
	std::copy( S.col_idx.begin(), S.col_idx.end(), tmp3 );
	cudaMemcpy( dev_col_idx, tmp3, S.col_idx.size()*sizeof(size_t), cudaMemcpyHostToDevice );
	delete[] tmp3;

	size_t* tmp4 = new size_t[ S.col_idx.size() ];
	std::copy( MaskLine.begin(), MaskLine.end(), tmp4 );
	cudaMemcpy( dev_MaskLine, tmp4, MaskLine.size()*sizeof(size_t), cudaMemcpyHostToDevice );
	delete[] tmp4;

	RefNumberType* init_vec_data = new RefNumberType[n];
	for( int i = 0; i < n; ++ i )
	{
		init_vec_data[i] = InvFactor;
	}
	cudaMemcpy( dev_vec, init_vec_data, n*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	delete[] init_vec_data;

	RefNumberType* partial_tmpErr = new RefNumberType[BlocksPerGrid];

	while( tmpErr > eps )
	{

		// copy current state to old vector.
		cudaMemcpy( dev_vecOld, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToDevice );

		GPU_Reduce_PartialSum<<<1,1>>>( dev_vecOld, dev_MaskLine, MaskLine.size(), dev_partialSum, defaultValue );

		GPU_CSR_PR_PartialSum<<< (n+31)/32, 32>>>( dev_data, dev_row_ptr, dev_col_idx, dev_vecOld, dev_partialSum, dev_vec, d, n );

		assert( THREADS_PER_BLOCK == PowerOfTwoAlign( THREADS_PER_BLOCK ) );
		GPU_Reduce_Error_2<<< BlocksPerGrid, THREADS_PER_BLOCK >>>( dev_partial_tmpErr, dev_vecOld, dev_vec, n);

		//------------------------------------------------------------------
		// GPU DOWNLOAD AND COMPLETE ERROR REDUCTION
		//------------------------------------------------------------------
		tmpErr = 0;
		cudaMemcpy( partial_tmpErr, dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType), cudaMemcpyDeviceToHost );

		for( unsigned i = 0; i<BlocksPerGrid; ++i)
		{
			tmpErr += partial_tmpErr[i];
		}
		tmpErr = sqrt( tmpErr );
		printf("[k = %u]: %e\n", k++, tmpErr );

		// if( k > 1000 ) break;
	}
	delete[] partial_tmpErr;

	//------------------------------------------------------------------
	// GPU DOWNLOAD
	//------------------------------------------------------------------
	RefNumberType* tmpData = new RefNumberType[n];
	cudaMemcpy( tmpData, dev_vec, n*sizeof(RefNumberType), cudaMemcpyDeviceToHost );
	std::vector<RefNumberType> ret;
	ret.assign(tmpData, tmpData + n);
	delete[] tmpData;
	//------------------------------------------------------------------
	// GPU CLEANUP
	//------------------------------------------------------------------
	cudaFree( dev_data );
	cudaFree( dev_row_ptr );
	cudaFree( dev_col_idx );
	cudaFree( dev_MaskLine );
	cudaFree( dev_vec );
	cudaFree( dev_vecOld );
	cudaFree( dev_partial_tmpErr );
	cudaFree( dev_partialSum );
	
	return ret;
}

//------------------------------------------------------------------
// THIS FUNCION IS ONLY USED FOR TESTABILITY WHILE DEVELOPMENT
//------------------------------------------------------------------
RefNumberType TestWrapper_KERNEL_ReduceError( RefNumberType* init_vec_1, RefNumberType* init_vec_2, int n, int THREADS)
{
	int BlocksPerGrid = (n+THREADS-1)/THREADS;
	//------------------------------------------------------------------
	// GPU MEMORY
	//------------------------------------------------------------------
	RefNumberType *dev_vec;
	cudaMalloc( (void**) &dev_vec, n*sizeof(RefNumberType) );

	RefNumberType *dev_vecOld;
	cudaMalloc( (void**) &dev_vecOld, n*sizeof(RefNumberType) );

	RefNumberType *dev_partial_tmpErr;
	cudaMalloc( (void**) &dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType) );

	//------------------------------------------------------------------
	// GPU UPLOAD
	//------------------------------------------------------------------
	cudaMemcpy( dev_vec, init_vec_1, n*sizeof(RefNumberType), cudaMemcpyHostToDevice );
	cudaMemcpy( dev_vecOld, init_vec_2, n*sizeof(RefNumberType), cudaMemcpyHostToDevice );


	// GPU_Reduce_Error<<<1,1>>>( dev_tmpErr, dev_vecOld, dev_vec, n);
	assert( THREADS == PowerOfTwoAlign( THREADS ) );
	GPU_Reduce_Error_2<<< BlocksPerGrid, THREADS >>>( dev_partial_tmpErr, dev_vecOld, dev_vec, n);

	//------------------------------------------------------------------
	// GPU DOWNLOAD
	//------------------------------------------------------------------
	RefNumberType tmpErr = 0;
	RefNumberType* partial_tmpErr = new RefNumberType[BlocksPerGrid];
	cudaMemcpy( partial_tmpErr, dev_partial_tmpErr, BlocksPerGrid*sizeof(RefNumberType), cudaMemcpyDeviceToHost );

	for( unsigned i = 0; i<BlocksPerGrid; ++i)
	{
		tmpErr += partial_tmpErr[i];
	}

	delete[] partial_tmpErr;
	//------------------------------------------------------------------
	// GPU CLEANUP
	//------------------------------------------------------------------
	cudaFree( dev_vec );
	cudaFree( dev_vecOld );
	cudaFree( dev_partial_tmpErr );

	return tmpErr;
}

