#include <cmath>
#include <cassert>
#include <chrono>
#include "kernels.h"

#include "IO.hpp"
#include "Show.hpp"


/**
* Main Page rank entry routine.
* @param std::vector<std::vector<RefNumberType> > Matrix that defines the Graph
* @param RefNumberType The dumping factor d in [0,1] values around 0.85 are good
* @param RefNumberType The target precision eps
* @param unsigned  The mode that selects the kernel to be used for computation
* @param *double   The elapsed time of the kernel call measured in [ms].
* mode=0: 	default dense MatrixVector based iteration
* mode=1: 	sparse MatrixVector based iteration
* mode=2: 	sparse MatrixVector based iteration with hand optimized code (some of the updates (PageRank specific) can be done inside the loop)
* mode=10:  openMP version of 0
* mode=11: 	openMP version of 1
* mode=12:  openMP version of 2
*/

std::vector<RefNumberType> PageRank( std::vector<std::vector<RefNumberType> > &A, RefNumberType d, RefNumberType eps, unsigned mode, double *time, int Nthr )
{
	std::vector<RefNumberType> ret;

	if( mode == 0 || mode == 10 )
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		auto t_start = std::chrono::high_resolution_clock::now();
		//----------------------------------------------------------------------
		if( mode == 0) ret = PageRank_Dense( A, d, eps );
		else ret = PageRank_Dense_OMP( A, d, eps, Nthr );
		//----------------------------------------------------------------------
		auto t_end = std::chrono::high_resolution_clock::now();
		(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
	}else if( mode == 1 || mode == 11)
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		auto t_start = std::chrono::high_resolution_clock::now();
		//----------------------------------------------------------------------
		if( mode == 1) ret = PageRank_CSR( S, A.size(), d, eps );
		else ret = PageRank_CSR_OMP( S, A.size(), d, eps, Nthr );
		//----------------------------------------------------------------------
		auto t_end = std::chrono::high_resolution_clock::now();
		(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
	}else if( mode == 2 || mode == 12 )
	{
		std::vector<size_t> MaskLine = NormalizeRows( A );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		auto t_start = std::chrono::high_resolution_clock::now();
		//----------------------------------------------------------------------
		if( mode == 2) ret = PageRank_CSR_OPT( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps  );
		else ret = PageRank_CSR_OPT_OMP( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps, Nthr );
		//----------------------------------------------------------------------
		auto t_end = std::chrono::high_resolution_clock::now();
		(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
	}else if( mode == 100 )
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		#ifdef GPU
			auto t_start = std::chrono::high_resolution_clock::now();
			//----------------------------------------------------------------------
			ret = GPU_PageRank_Dense( A, d, eps );
			//----------------------------------------------------------------------
			auto t_end = std::chrono::high_resolution_clock::now();
			(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
			// printf("PRANK call duartion: %.2f ms \n" , (*time) );
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif

	}else if( mode == 101)
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		#ifdef GPU
			auto t_start = std::chrono::high_resolution_clock::now();
			//----------------------------------------------------------------------
			ret =  GPU_PageRank_CSR( S,  A.size(), d, eps );
			//----------------------------------------------------------------------
			auto t_end = std::chrono::high_resolution_clock::now();
			(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif
	}else if( mode == 102)
	{
		std::vector<size_t> MaskLine = NormalizeRows( A );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		#ifdef GPU
			auto t_start = std::chrono::high_resolution_clock::now();
			//----------------------------------------------------------------------
			ret = GPU_PageRank_CSR_OPT( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps );
			//----------------------------------------------------------------------
			auto t_end = std::chrono::high_resolution_clock::now();
			(*time) = std::chrono::duration<double, std::milli>(t_end-t_start).count();
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif
	} else
	{
		printf("mode = %u not supported\n", mode);
		exit(-1);
	}

	return ret;
}

std::vector<size_t> NormalizeRows( std::vector<std::vector<RefNumberType> > &A, RefNumberType defaultValue )
{
	std::vector<size_t> ret;
	for( size_t row = 0; row < A.size(); ++row)
	{
		double sum = 0;
		for( size_t j = 0; j<A[row].size(); ++j )
		{
			sum += A[row][j];
		}

		if( sum == 0 )
		{
			// can not normalize
			// update MaskSet and 
			// set default values
			ret.push_back( row );

			for( unsigned j = 0; j<A[row].size(); ++j )
			{
				A[row][j] = defaultValue;
			}
		}else
		{
			// Regular row - normalization
			for( size_t j = 0; j<A[row].size(); ++j )
			{
				A[row][j] /= sum;
			}
		}
	}
	return ret;
}

void Tp( std::vector<std::vector<RefNumberType> > &A )
{
	//squared matrix required (n x n)
	assert( A.size() == A[0].size() );
	size_t n = A.size();
	RefNumberType tmp = -1;

	for( size_t i = 0; i<n; ++i )
	{
		for( size_t j=i+1; j<n; ++j )
		{
			tmp 	= A[i][j];
			A[i][j] = A[j][i];
			A[j][i] = tmp;
		}
	}
}

CSRType<RefNumberType> Dense2Sparse( std::vector<std::vector<RefNumberType> > const &A )
{
	CSRType<RefNumberType> ret;

	size_t n = A.size();

	ret.row_ptr.resize( n + 1 );

	unsigned nzz = 0;

	for( size_t i = 0; i<n; ++i )
	{
		ret.row_ptr[i] = nzz;

		for( size_t j=0; j<n; ++j )
		{
			if( A[i][j] != 0)
			{
				// non zero element, add it.
				ret.data.push_back( A[i][j] );
				ret.col_idx.push_back( j );
				nzz++;
			}
		}
	}

	ret.row_ptr[n] = nzz;

	return ret;
}

std::vector<std::vector<RefNumberType> > Sparse2Dense( CSRType<RefNumberType> const &S, size_t n)
{
	std::vector<std::vector<RefNumberType> > ret;

	for( size_t i = 0; i<n; ++i )
	{
		ret.push_back( std::vector<RefNumberType>(n, 0 ) );

		for( size_t idx = S.row_ptr[i]; idx < S.row_ptr[i+1]; ++idx )
		{
			ret[i][ S.col_idx[idx] ] = S.data[idx];
		}
	}

	return ret;
}

void showVec( std::vector<RefNumberType>& vec)
{ 
	for( int i = 0; i < vec.size(); ++ i )
	{
		printf("\t%.10e", vec[i]);
	}
	printf("\n");
}

std::vector<RefNumberType> PageRank_Dense( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps )
{
	//squared matrix required (n x n)
	assert( A.size() == A[0].size() );

	unsigned k 			= 0;
	size_t n 			= A.size();
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);

	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(n^2): dense matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );

		// showVec( pold );

		for( size_t row = 0; row < n; ++row)
		{
			double sum = 0;
			for( size_t j = 0; j< n; ++j )
			{
				sum += A[row][j]*pold[j];
			}
			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// finish computation of norm( pnext - pold)
		tmpErr = sqrt( tmpErr ); 
		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

std::vector<RefNumberType> PageRank_Dense_OMP( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps, int Nthr )
{
	//squared matrix required (n x n)
	assert( A.size() == A[0].size() );

	unsigned k 			= 0;
	size_t n 			= A.size();
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);

	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(n^2): dense matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );
    	
    	double sum;
    	size_t j;

    	#pragma omp parallel for private(sum, j) num_threads(Nthr) // use num_threads(x) to specify the number of threads. 
		for( size_t row = 0; row < n; ++row)
		{
			sum = 0;
			for(j = 0; j< n; ++j )
			{
				sum += A[row][j]*pold[j];
			}
			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;
			
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			// #pragma omp critical
			// tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// reduction shoud be done in order to have the same results (e.g. loop count)
		for( size_t row = 0; row < n; ++row)
		{
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// finish computation of norm( pnext - pold)
		tmpErr = sqrt( tmpErr ); 
		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

std::vector<RefNumberType> PageRank_CSR( CSRType<RefNumberType> const &S, size_t n,  RefNumberType d, RefNumberType eps )
{
	unsigned k 			= 0;
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);
	
	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(nnz of sparse matrix): sparse matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );

		for( size_t row = 0; row < n; ++row)
		{
			double sum = 0;

			for( size_t idx = S.row_ptr[row]; idx < S.row_ptr[row+1]; ++idx )
			{
				sum += S.data[idx]*pold[ S.col_idx[idx] ];
			}

			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// finish computation of norm( pold - pnext)
		tmpErr = sqrt( tmpErr ); 

		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

std::vector<RefNumberType> PageRank_CSR_OMP( CSRType<RefNumberType> const &S, size_t n,  RefNumberType d, RefNumberType eps, int Nthr  )
{
	unsigned k 			= 0;
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);
	
	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(nnz of sparse matrix): sparse matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );

		double sum ;

		#pragma omp parallel for private(sum) num_threads(Nthr)
		for( size_t row = 0; row < n; ++row)
		{
			sum = 0;

			for( size_t idx = S.row_ptr[row]; idx < S.row_ptr[row+1]; ++idx )
			{
				sum += S.data[idx]*pold[ S.col_idx[idx] ];
			}

			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;

			// #pragma omp critical
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			// tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// reduction shoud be done in order to have the same results (e.g. loop count)
		for( size_t row = 0; row < n; ++row)
		{
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		} 

		// finish computation of norm( pold - pnext)
		tmpErr = sqrt( tmpErr ); 

		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

std::vector<RefNumberType> PageRank_CSR_OPT( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps )
{
	unsigned k 			= 0;
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);
	
	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(nnz of sparse matrix): sparse matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );

		// We have A = Asparse + Aline
		// A DOT p = Asparse DOT p + Aline DOT p
		// compute the part Aline DOT p (it is only one scalar product, since all rows of Aline are equivalent)
		double partialSum = 0;
		for( size_t i = 0; i<MaskLine.size(); ++i)
		{
			partialSum += pold[ MaskLine[i] ];
		}
		partialSum *= defaultValue;
		//printf("psum = %f\n", partialSum );

		// Add the computation of Asparse DOT p and compute the final value of A DOT p.
		for( size_t row = 0; row < n; ++row)
		{
			double sum = partialSum;

			for( size_t idx = S.row_ptr[row]; idx < S.row_ptr[row+1]; ++idx )
			{
				sum += S.data[idx]*pold[ S.col_idx[idx] ];
			}

			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// finish computation of norm( pold - pnext)
		tmpErr = sqrt( tmpErr ); 

		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

std::vector<RefNumberType> PageRank_CSR_OPT_OMP( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps, int Nthr )
{
	unsigned k 			= 0;
	double InvFactor 	= 1/((double) n);
	double tmpErr 		= 2*eps;

	// uniform vecotor of length n, with values 1/n at all positions
	std::vector<RefNumberType> ret( n, InvFactor);
	
	while( tmpErr > eps )
	{
		// compute p = dAp  + (1-d).*1/n.*[1 1 ... 1];
		// O(nnz of sparse matrix): sparse matrix-vector multiplication

		tmpErr = 0;
		std::vector<RefNumberType> pold( ret );

		// We have A = Asparse + Aline
		// A DOT p = Asparse DOT p + Aline DOT p
		// compute the part Aline DOT p (it is only one scalar product, since all rows of Aline are equivalent)
		double partialSum = 0;
		for( size_t i = 0; i<MaskLine.size(); ++i)
		{
			partialSum += pold[ MaskLine[i] ];
		}
		partialSum *= defaultValue;
		//printf("psum = %f\n", partialSum );

		double sum;

		// Add the computation of Asparse DOT p and compute the final value of A DOT p.
		#pragma omp parallel private(sum) num_threads(Nthr)
		for( size_t row = 0; row < n; ++row)
		{
			sum = partialSum;

			for( size_t idx = S.row_ptr[row]; idx < S.row_ptr[row+1]; ++idx )
			{
				sum += S.data[idx]*pold[ S.col_idx[idx] ];
			}

			// sum now contains the scalar product A[row,:] DOT p
			ret[row] 	= d*sum + (1-d)*InvFactor;
			
			// #pragma omp critical
			// on the fly compute norm( pold - pnext, 2) i.e. L2-norm between the current and last iteration
			// tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		}

		// reduction shoud be done in order to have the same results (e.g. loop count)
		for( size_t row = 0; row < n; ++row)
		{
			tmpErr 	   += (ret[row]-pold[row])*(ret[row]-pold[row]);
		} 

		// finish computation of norm( pold - pnext)
		tmpErr = sqrt( tmpErr ); 

		printf("[k = %u]: %e\n", k++, tmpErr );
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////
// GPU HELPER CODE 
/////////////////////////////////////////////////////////////////////////////

void MatrixToFlatData( RefNumberType* const dataA, std::vector<std::vector<RefNumberType> > const &A)
{
	unsigned rowoffset = 0;
	for( size_t r = 0; r<A.size(); ++r )
	{
		for( size_t c = 0; c<A[r].size(); ++c )
		{
			dataA[rowoffset + c] = A[r][c];
		}

		rowoffset += A[r].size();
	}
}

std::vector<std::vector<RefNumberType> > FlatDataToMatrix( const RefNumberType* const dataA, int Ndim )
{
	std::vector<std::vector<RefNumberType> > ret = InitZeros<RefNumberType>( Ndim );

	for( size_t r = 0; r<Ndim; ++r )
	{
		for( size_t c = 0; c<Ndim; ++c )
		{
			ret[r][c] = dataA[ Ndim*r + c];
		}
	}	

	return ret;
}
