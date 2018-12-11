#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cassert>
#include <gtest/gtest.h>

#include <random>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"
#include "Check.hpp"
#include "GenzFunctions.hpp"

class QLI_TestFixture1: public ::testing::TestWithParam<std::tr1::tuple<int, int, double, double, int> >
{
	protected: 

    QLI_TestFixture1( ) { 
    } 
    
    virtual void SetUp( ) { 
    }
 
    virtual void TearDown( ) { 
    }
 
    ~QLI_TestFixture1( )  { 
    } 
 };

TEST_P(QLI_TestFixture1, Test1)
{
	const double NEAR_THR   = 1e-15;
    const int Nintervalls   = 1000;

	DataHandle_1D_Type<double> data_1d;
	
	int Npoints 		= std::tr1::get<0>(GetParam());
    data_1d.FunctionId 	= std::tr1::get<1>(GetParam());
    data_1d.u     		= std::tr1::get<2>(GetParam());
    data_1d.a 			= std::tr1::get<3>(GetParam());
    int Nthr 			= std::tr1::get<4>(GetParam());

    double r 	= gauss_legendre_splitEqualIntervalls_OMP( Nintervalls, Npoints, f_handle_1d, &data_1d, 0, 1, Nthr);
    double ref 	= RefIntegeral( data_1d.FunctionId, data_1d.u, data_1d.a );

    printf("CHECK: %f %f %e\n", r, ref, r-ref );
	EXPECT_NEAR( r, ref, NEAR_THR );
}

INSTANTIATE_TEST_CASE_P(
  CheckAllFunctions, QLI_TestFixture1, testing::Values(
    //           		   Npoints	FuncId 	u 		a 	  Nthreads.
    ::testing::make_tuple( 1024,	1,		0.3,	0.4,  5	),    
    ::testing::make_tuple( 1024,	2,		0.3,	0.4,  5	), 
    ::testing::make_tuple( 1024,	3,		0.3,	0.4,  5	), 
    ::testing::make_tuple( 1024,	4,		0.3,	0.4,  5	), 
    ::testing::make_tuple( 1024,	5,		0.3,	0.4,  5	), 
    ::testing::make_tuple( 1024,	6,		0.3,	0.4,  5	),
    ::testing::make_tuple( 1024,	1,		0.3,	0.4,  20 ), 
    ::testing::make_tuple( 1024,	2,		0.3,	0.4,  20 ), 
    ::testing::make_tuple( 1024,	3,		0.3,	0.4,  20 ), 
    ::testing::make_tuple( 1024,	4,		0.3,	0.4,  20 ), 
    ::testing::make_tuple( 1024,	5,		0.3,	0.4,  20 ), 
    ::testing::make_tuple( 1024,	6,		0.3,	0.4,  20 )
)); 

INSTANTIATE_TEST_CASE_P(
  CheckAllFunctions_002, QLI_TestFixture1, testing::Values(
    //           		   Npoints	FuncId 	u 		a 	    Nthreads
    ::testing::make_tuple( 1024,	1,		0.3,	2.222, 	5 ), 
    ::testing::make_tuple( 1024,	2,		0.3,	2.222, 	5 ), 
    ::testing::make_tuple( 1024,	3,		0.3,	2.222, 	5 ), 
    ::testing::make_tuple( 1024,	4,		0.3,	2.222, 	5 ), 
    ::testing::make_tuple( 1024,	5,		0.3,	2.222, 	5 ), 
    ::testing::make_tuple( 1024,	6,		0.3,	2.222, 	5 ),
    ::testing::make_tuple( 1024,	1,		0.3,	2.222, 	20 ), 
    ::testing::make_tuple( 1024,	2,		0.3,	2.222, 	20 ), 
    ::testing::make_tuple( 1024,	3,		0.3,	2.222, 	20 ), 
    ::testing::make_tuple( 1024,	4,		0.3,	2.222, 	20 ), 
    ::testing::make_tuple( 1024,	5,		0.3,	2.222, 	20 ), 
    ::testing::make_tuple( 1024,	6,		0.3,	2.222, 	20 )
)); 

class QLI_TestFixture2: public ::testing::TestWithParam<std::tr1::tuple<int, double, int> >{};

TEST_P(QLI_TestFixture2, Test_Run_Params)
{

	int FnId 		= std::tr1::get<0>(GetParam());
	double NEAR_THR = std::tr1::get<1>(GetParam());
    int Nthr 		= std::tr1::get<2>(GetParam());


	int Nsplits  = 1000;  // Number of sub-intervalls
	int Npoints  = 1024; // GLI support points
	int Nrep_max = 100;  // number of repetitions with random parameters
    // Random generator and seed.
    int seed = 0; 
    std::mt19937 rng(seed);

    // uniform distribution geneator in [0,1]
    std::uniform_real_distribution<double> uniform_u(0, 1);
    std::uniform_real_distribution<double> uniform_a(0, 1);

	DataHandle_1D_Type<double> data_1d;


	for( int Nrep = 0; Nrep < Nrep_max; ++ Nrep )
	{
    	data_1d.FunctionId 	= FnId;
	    data_1d.u     		= (double) uniform_u(rng);
	    data_1d.a 			= (double) uniform_a(rng);

	    double r 	= gauss_legendre_splitEqualIntervalls_OMP( Nsplits, Npoints, f_handle_1d, &data_1d, 0, 1, Nthr );
	    double ref 	= RefIntegeral( data_1d.FunctionId, data_1d.u, data_1d.a );

	    // printf("CHECK: (u=%f, a=%f): %f %f %e\n", data_1d.u, data_1d.a, r, ref, r-ref );
		EXPECT_NEAR( r, ref, NEAR_THR );
	}
}

// NOTE case 5 and 6 are harder by construction, hence the error tolerance is higher.
// NOTE, the discontinuity causes the integration to introduce a relative big error in the last case.

INSTANTIATE_TEST_CASE_P(
  CheckAllFunctions, QLI_TestFixture2, testing::Values(
    //           		   Npoints	FuncId 	u 		a 	
    ::testing::make_tuple( 1, 1e-14, 5 ), 
    ::testing::make_tuple( 2, 1e-14, 5 ), 
    ::testing::make_tuple( 3, 1e-14, 5 ), 
    ::testing::make_tuple( 4, 1e-14, 5 ), 
    ::testing::make_tuple( 5, 1e-10, 5 ),  
    ::testing::make_tuple( 6, 1e-5, 20 ), 
    ::testing::make_tuple( 1, 1e-14, 20 ), 
    ::testing::make_tuple( 2, 1e-14, 20 ), 
    ::testing::make_tuple( 3, 1e-14, 20 ), 
    ::testing::make_tuple( 4, 1e-14, 20 ), 
    ::testing::make_tuple( 5, 1e-10, 20 ),  
    ::testing::make_tuple( 6, 1e-5, 20 ) 
)); //*/

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}