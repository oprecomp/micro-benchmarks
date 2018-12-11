// ########################################################################
// ### PROJECT OPRECOMP 												###
// ###------------------------------------------------------------------###
// ### Purpose:	Kernel functions for MB PageRank 						###
// ###			- there are 6 variations of the code present:			###
// ### 			 	1) an implementation for a dense matrix 			###
// ###				2) an implementation for a sparse matrix 			###
// ###				  	based on CSR MAT-VEC multiplication				###
// ###				3) an implementation for a sparse matrix 			###
// ###					 using an hand-optimized optimization for the 	###
// ###					 innermost iteration kernel suited for PageRank	###
// ###				4)-6) openMP parallel verison of 1)-3)				###
// ###------------------------------------------------------------------###
// ### PROJECT MANAGEMENT:			GIT RESPONSIBLE: 					###
// ### ----------------------		----------------------------------- ###
// ### Cristiano Malossi		 	1.) 	Fabian Schuiki		        ###
// ### IBM Research - Zurich 				ETH Zurich 					###
// ### Research Staff Member 	 			fschuiki@iis.ee.ethz.ch  	###
// ### acm@zurich.ibm.com    		2.) 	Florian Scheidegger 		###
// ### +41(0)44 724 8616					IBM Research - Zurich  		###
// ### 										eid@zurich.ibm.com 			###
// ########################################################################

#pragma once

// ########################################################################
// INCLUDES
// ########################################################################
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// ########################################################################
// GENERIC DATATYPES 
// ########################################################################
// container to store sparse data in compressed sparse row (CSR) format
template<typename T>
struct CSRType
{
	std::vector<T> 	 		data;
	std::vector<size_t>  	row_ptr;
	std::vector<size_t>   	col_idx;
};

// ########################################################################
// Basic Numeric Datatypes
// ########################################################################
// That line defines the datatype that is used for computing reference values of the Micro Benchmarks.
// We consider two types:
// 	- IEEE 754 single precision (32bit), used with "float" keyword in C
// 	- IEEE 754 double precision (64bit), used with "double" keyword in C
// Please use RefNumberType instead of float or double to easily and consistenly switch through the project.
typedef double RefNumberType;


// Main Entry Point
std::vector<RefNumberType> PageRank( std::vector<std::vector<RefNumberType> > &A, RefNumberType d, RefNumberType eps, unsigned mode, double *time, int Nthr );

// HELPER FUNCITONS
std::vector<size_t> NormalizeRows( std::vector<std::vector<RefNumberType> > &A, RefNumberType defaultValue = 0.0);

void Tp( std::vector<std::vector<RefNumberType> > &A );

CSRType<RefNumberType> Dense2Sparse( std::vector<std::vector<RefNumberType> > const &A );
std::vector<std::vector<RefNumberType> > Sparse2Dense( CSRType<RefNumberType> const &S, size_t n);

// MAIN PAGERANK KERNELS
std::vector<RefNumberType> PageRank_Dense( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps );

std::vector<RefNumberType> PageRank_CSR( CSRType<RefNumberType> const &S, size_t n,  RefNumberType d, RefNumberType eps );

std::vector<RefNumberType> PageRank_CSR_OPT( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps );

// OMP OPTIMIZED KERNELS
std::vector<RefNumberType> PageRank_Dense_OMP( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps, int Nthr );
std::vector<RefNumberType> PageRank_CSR_OMP( CSRType<RefNumberType> const &S, size_t n,  RefNumberType d, RefNumberType eps, int Nthr );
std::vector<RefNumberType> PageRank_CSR_OPT_OMP( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps, int Nthr );


// Flatten Data
void MatrixToFlatData( RefNumberType* const dataA, std::vector<std::vector<RefNumberType> > const &A); 
std::vector<std::vector<RefNumberType> > FlatDataToMatrix( const RefNumberType* const dataA, int Ndim );
unsigned int PowerOfTwoAlign( unsigned int n );

#ifdef GPU

std::vector<RefNumberType> GPU_PageRank_Dense( std::vector<std::vector<RefNumberType> > const &A, RefNumberType d, RefNumberType eps );
std::vector<RefNumberType> GPU_PageRank_CSR( CSRType<RefNumberType> const &S, size_t n, RefNumberType d, RefNumberType eps );
std::vector<RefNumberType> GPU_PageRank_CSR_OPT( CSRType<RefNumberType> const &S, size_t n,  std::vector<size_t> const &MaskLine, RefNumberType defaultValue, RefNumberType d, RefNumberType eps );

RefNumberType TestWrapper_KERNEL_ReduceError( RefNumberType* init_vec_1, RefNumberType* init_vec_2, int n, int THREADS);

#endif
