// ########################################################################
// ### PROJECT OPRECOMP 												###
// ###------------------------------------------------------------------###
// ### Purpose:	glq micro Benchmark 									###
// ### 			using library calls to compute integrals 				###
// ###			Genz functions are used to test the kernels 			###
// ###			parallelisem is invoked by splitting the integrand in  	###
// ### 			interalls  												###
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

double gauss_legendre_splitEqualIntervalls( int intervall_n, int GLI_n, double (*f)(double,void*), void* data, double a, double b);

double gauss_legendre_splitEqualIntervalls_OMP( int intervall_n, int GLI_n, double (*f)(double,void*), void* data, double a, double b, int Nthr);

