#include <cmath>
#include <cassert>
#include <vector>
#include "kernels.h"
#include "gauss_legendre.h"

double gauss_legendre_splitEqualIntervalls( int intervall_n, int GLI_n, double (*f)(double,void*), void* data, double a, double b)
{
	double r = 0;
	double delta = (b-a)/(double) intervall_n;

	// equald split of interval [a,b]
	for( int i = 0; i<intervall_n; ++i )
	{
		r += gauss_legendre( GLI_n, f, data, a+i*delta, a+(i+1)*delta );
	}
	return r;
}

double gauss_legendre_splitEqualIntervalls_OMP( int intervall_n, int GLI_n, double (*f)(double,void*), void* data, double a, double b, int Nthr)
{
	double delta = (b-a)/(double) intervall_n;

	std::vector<double> tmpr = std::vector<double>( intervall_n );

	// equald split of interval [a,b]
	#pragma omp parallel for num_threads(Nthr) // use num_threads(x) to specify the number of threads. 
	for( int i = 0; i<intervall_n; ++i )
	{
		tmpr[i] = gauss_legendre( GLI_n, f, data, a+i*delta, a+(i+1)*delta );
	}

	double r = 0;
	// synchronized merge of results.
	for( int i = 0; i<intervall_n; ++i )
	{
		r+= tmpr[i];
	}

	return r;
}


