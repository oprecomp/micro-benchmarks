#include <cmath>
#include <stdint.h>
#include <ctime>
#include <chrono>

#include <vector>

#include <omp.h>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"

/////////////////////////////////////////////////////////////////////////////
// GRIZZLY (MAC-OS with openMP compiler)
/////////////////////////////////////////////////////////////////////////////
// clang-omp++ -std=c++11 -c ../lib/kernels.cpp
// clang-omp++ -std=c++11 -I ../lib/ -I ../util/ run.cpp kernels.o 


/////////////////////////////////////////////////////////////////////////////
// SCALIGERA
/////////////////////////////////////////////////////////////////////////////
// g++ -c ../lib/kernels.cpp 
// g++ -std=c++11 -I ../lib/ -I ../util/ run.cpp kernels.o


/////////////////////////////////////////////////////////////////////////////
// GPU compile (on SCALIGERA)
/////////////////////////////////////////////////////////////////////////////
// cd  
// nvcc -DGPU -c -I ../util/ ../lib/kernels.cu
// 

double avg( std::vector<double> &vec)
{
	double sum = 0;
	for(size_t i = 0; i<vec.size(); ++i)
	{
		sum += vec[i];
	}
	return sum / ((double ) vec.size());
}

double stdev( std::vector<double> &vec)
{
	double mu = avg(vec);
	double sum = 0;
	for(size_t i = 0; i<vec.size(); ++i)
	{
		sum += (mu - vec[i])*(mu - vec[i]);
	}
	sum = sum / ((double ) vec.size() - 1);
	return sqrt(sum);
}

////////////////////////////////////////////////////////////////////////////////////////////////
/// SOME BASIC RESULTS:
////////////////////////////////////////////////////////////////////////////////////////////////
// mode=  0:   51209.56 ms +/- 51.40 ms            |51190.13       51267.84        51170.71
// mode=  1:   20743.13 ms +/- 5.61 ms             |20749.25       20741.92        20738.23
// mode=  2:     182.92 ms +/- 0.08 ms             |  182.90         182.84          183.00
// mode= 10:    2405.54 ms +/- 244.08 ms           | 2680.25        2322.79         2213.60
// mode= 11:     988.90 ms +/- 168.09 ms           |  803.91        1030.52         1132.28
// mode= 12:     241.75 ms +/- 31.06 ms            |  254.69         264.26          206.32
// mode=100:     599.43 ms +/- 197.57 ms           |  827.52         481.54          489.22
// mode=101:     157.46 ms +/- 12.41 ms            |  149.85         150.75          171.78
// mode=102:      83.43 ms +/- 0.88 ms             |   82.79          84.44           83.08
////// FIXED AMOUNT OF THREADS for openMP.
// mode= 10, Nthr=  1:   52885.29 ms +/- 20.48 ms 		|52906.42	52883.95	52865.52	
// mode= 10, Nthr=  4:   13915.46 ms +/- 214.44 ms 	|14004.20	14071.29	13670.90	
// mode= 10, Nthr= 32:    3599.19 ms +/- 96.45 ms 		| 3612.99	 3496.59	 3688.00	
// mode= 10, Nthr=128:    2146.18 ms +/- 395.58 ms 	| 2597.55	 1981.20	 1859.78	
// mode= 10, Nthr=512:    1740.99 ms +/- 42.23 ms 		| 1698.60	 1783.06	 1741.30	
// mode= 11, Nthr=  1:   21923.79 ms +/- 1.83 ms 		|21925.90	21922.83	21922.62	
// mode= 11, Nthr=  4:    5868.27 ms +/- 165.44 ms 	| 5760.41	 6058.75	 5785.66	
// mode= 11, Nthr= 32:    1524.99 ms +/- 31.84 ms 		| 1543.84	 1488.23	 1542.90	
// mode= 11, Nthr=128:     974.31 ms +/- 167.17 ms 	| 1164.58	  850.99	  907.36	
// mode= 11, Nthr=512:     859.99 ms +/- 9.88 ms 		|  850.89	  870.50	  858.60	
// mode= 12, Nthr=  1:     191.45 ms +/- 0.02 ms 		|  191.44	  191.47	  191.45	
// mode= 12, Nthr=  4:     198.41 ms +/- 1.22 ms 		|  198.56	  199.55	  197.13	
// mode= 12, Nthr= 32:     384.70 ms +/- 5.38 ms 		|  379.53	  390.26	  384.32	
// mode= 12, Nthr=128:     847.90 ms +/- 101.76 ms 	|  965.39	  787.16	  791.16	
// mode= 12, Nthr=512:    2523.66 ms +/- 24.59 ms 		| 2499.11	 2523.56	 2548.30

void RoughTiming()
{
	RefNumberType const d     	= 0.9;
    RefNumberType const eps   	=1e-14; // double 
    //RefNumberType const eps   	=1e-5; 	// float 

	// std::vector<unsigned> modes;
	// modes.push_back(0);
	// modes.push_back(1);
	// modes.push_back(2);

	// modes.push_back(10);
	// modes.push_back(11);
	// modes.push_back(12);

	// modes.push_back(100);
	// modes.push_back(101);
	// modes.push_back(102);

	// std::vector<int> Nthrs(160, modes.size());

	std::vector<unsigned> modes;
	std::vector<int> Nthrs; 

	modes.push_back(10); 	Nthrs.push_back(1);
	modes.push_back(10); 	Nthrs.push_back(4);
	modes.push_back(10); 	Nthrs.push_back(32);
	modes.push_back(10); 	Nthrs.push_back(128);
	modes.push_back(10); 	Nthrs.push_back(512);

	modes.push_back(11); 	Nthrs.push_back(1);
	modes.push_back(11); 	Nthrs.push_back(4);
	modes.push_back(11); 	Nthrs.push_back(32);
	modes.push_back(11); 	Nthrs.push_back(128);
	modes.push_back(11); 	Nthrs.push_back(512);

	modes.push_back(12); 	Nthrs.push_back(1);
	modes.push_back(12); 	Nthrs.push_back(4);
	modes.push_back(12); 	Nthrs.push_back(32);
	modes.push_back(12); 	Nthrs.push_back(128);
	modes.push_back(12); 	Nthrs.push_back(512);

	const unsigned runs = 3;

	std::vector< std::vector<double> > times;
	times.resize(modes.size());

	for( unsigned run = 0; run < runs; ++run )
	{
		for( size_t i = 0; i<modes.size(); ++i )
		{

			std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/dataL/eid/GIT/oprecomp/mb/pagerank/build/data/prepared/mb/pagerank/_abortion/graph/adj_matrix", ','); 
			// std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/dataL/eid/GIT/oprecomp/mb/pagerank/testsuite/data/float_in000.csv", ','); 

			double time;
			std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) modes[i], &time, Nthrs[i]);
			printf(" %.2f ms " , time );

			times[i].push_back(time);

			// for( size_t i = 0; i<ret0.size(); ++i)
			// {
			// 	printf("\t%.10e", (RefNumberType) ret0[i]);
			// }
			// printf("\n");
		}
	}

	// Show results
	for( size_t i = 0; i<modes.size(); ++i )
	{
		printf("mode=%3i, Nthr=%3i: %10.2f ms +/- %4.2f ms \t\t|", modes[i], Nthrs[i], avg(times[i]), stdev(times[i]));
		for( size_t j = 0; j<times[i].size(); ++j)
		{
			printf("%8.2f\t", times[i][j]);
		}
		printf("\n");
	}

	writeCSVMatrix( times, "times_measured.csv", ',');

}

void foo( int i )
{
	//printf("The value of i is: %i that corresponds to %.10e\n", i, (double) i );
	volatile int n;
	n = i*i;
}

int main(int argc, char **argv) {
    // if( argc != 3 )
    // {
    //     printf("Usage: ./MB001 <GTEST PARAMS> <DIRECTORY TO (SMALL) DATA> <DIR TO PAGERANK DATA>\n");
    //     exit(1);
    // }
/*
    //::testing::make_tuple( 0.9,  1e-14,	1, std::string("/float_in000.csv") ),
    RefNumberType const d     	= 0.9;
    //RefNumberType const eps   	=1e-14;
    RefNumberType const eps   	= 1e-5;

	// std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/Users/eid/Desktop/GIT/oprecomp/mb/pagerank/testsuite/data/float_in000.csv", ','); 
	// std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/dataL/eid/GIT/oprecomp/mb/pagerank/testsuite/data/float_in000.csv", ',');
	// std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/dataL/eid/GIT/oprecomp/mb/pagerank/testsuite/data/float_in000.csv", ',');
	std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( "/dataL/eid/GIT/oprecomp/mb/pagerank/build/data/prepared/mb/pagerank/_abortion/graph/adj_matrix", ','); 
	std::vector<std::vector<RefNumberType> > A2 = A;
	std::vector<std::vector<RefNumberType> > A3 = A;

	std::clock_t now = std::clock();
	auto t_start = std::chrono::high_resolution_clock::now();

	double time;
	std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 100, &time);
	
	std::clock_t stop = std::clock();
	double time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;

	auto t_end = std::chrono::high_resolution_clock::now();

	printf(" %.2f (INNER WALL) ms %.2f ms (CLOCK)  %.2fms (WALL)\n" , time, time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() );


	// for( size_t i = 0; i<ret0.size(); ++i)
	for( size_t i = 0; i<10; ++i)
	{
		printf("\t%.10e", (RefNumberType) ret0[i]);
	}
	printf("\n");

	now = std::clock();
	t_start = std::chrono::high_resolution_clock::now();

	ret0 = PageRank( A2, d, eps, (unsigned) 101, &time);
	
	stop = std::clock();
	time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;

	t_end = std::chrono::high_resolution_clock::now();

	printf(" %.2f (INNER WALL) ms %.2f ms (CLOCK)  %.2fms (WALL)\n" , time, time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() );

	// for( size_t i = 0; i<ret0.size(); ++i)
	for( size_t i = 0; i<10; ++i)
	{
		printf("\t%.10e", (RefNumberType) ret0[i]);
	}
	printf("\n");

	now = std::clock();
	t_start = std::chrono::high_resolution_clock::now();

	ret0 = PageRank( A3, d, eps, (unsigned) 102, &time);
	
	stop = std::clock();
	time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;

	t_end = std::chrono::high_resolution_clock::now();

	printf(" %.2f (INNER WALL) ms %.2f ms (CLOCK)  %.2fms (WALL)\n" , time, time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() );


	// for( size_t i = 0; i<ret0.size(); ++i)
	for( size_t i = 0; i<10; ++i)
	{
		printf("\t%.10e", (RefNumberType) ret0[i]);
	}
	printf("\n"); // */

	RoughTiming();

	/* 
	// 1000 loop iterations, printing stuff, around 6 ms.
	// 1000 loop iterations, simple computations around 0.1ms.
	// time go-stop, 0.00 ms.

	now = std::clock();
	t_start = std::chrono::high_resolution_clock::now();

	/*
	for(int i = 0; i<1000; ++i)
	{
		//foo(i);
	} 

	stop = std::clock();
	time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;

	t_end = std::chrono::high_resolution_clock::now();

	printf(" %.2f (INNER WALL) ms %.2f ms (CLOCK)  %.2fms (WALL)\n" , time, time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() ); //*/

}

