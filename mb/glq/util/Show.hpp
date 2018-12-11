
#include <stdio.h>
#include <stdlib.h>
#include <vector>

template<typename T>
void PrintMatrix( 	const std::vector< std::vector<T> > &A,
					size_t row_start 	= 0, 	
					size_t row_end 		= 0,	
					size_t col_start 	= 0, 	
					size_t col_end  	= 0)	
{
	if(row_end == 0 ) row_end = A.size();
	if(col_end == 0 ) col_end = A[0].size();
	assert( row_end <= A.size());
	assert( col_end <= A[0].size());

	printf("Matrix %lu x %lu:\n", A.size(), A[0].size() );
	if( row_start > 0 ) printf("... rows 0 - %lu skipped ...\n", row_start-1 );
	for( size_t r = row_start; r<row_end; ++r)
	{
		assert( A[0].size() == A[r].size() );
		printf("[row %lu]", r);
		if( col_start != 0 ) printf("[...]\t");

		for( size_t c = col_start; c<col_end; ++c)
		{
			printf("\t%.e", (T) A[r][c]);
		}

		if( col_end < A[0].size() ) printf("\t[...]");
		printf("\n");

	}
	if( row_end < A.size() ) printf("... rows %lu - %lu skipped ...\n", row_end, A.size()-1 );
}

template<typename T>
void PrintCSR( CSRType<T> const &S, size_t n )
{
	for( size_t i = 0; i<n; ++i )
	{
		for( size_t idx = S.row_ptr[i]; idx < S.row_ptr[i+1]; ++idx )
		{
			// ret[i][ S.col_id[idx] ] = S.data[idx];
			printf("(%lu, %lu) = %f\n", i, S.col_idx[idx], S.data[idx] );
		}
	}
}