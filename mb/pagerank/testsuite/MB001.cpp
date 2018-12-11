#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cassert>
#include <gtest/gtest.h>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"
#include "Check.hpp"

// global arguments to access command line options.
int my_argc;
char** my_argv;

// ########################################################################
// PAGE RANK -- KERNELS
// ########################################################################

// HOW TO: 	https://github.com/google/googletest/blob/master/googletest/samples/sample3_unittest.cc
// AND: 	http://stackoverflow.com/questions/8971572/how-to-test-multi-parameter-formula
//
//class PageRank_TestFixture1: public ::testing::Test
//{ 

class PageRank_TestFixture1: public ::testing::TestWithParam<std::tr1::tuple<RefNumberType, RefNumberType, int, std::string> >
{
	protected: 
    	std::vector<std::vector<RefNumberType> > A;
    	std::vector<std::vector<RefNumberType> > B;
    	std::vector<std::vector<RefNumberType> > C;

    	std::vector<std::vector<RefNumberType> > D;
    	std::vector<std::vector<RefNumberType> > E;
    	std::vector<std::vector<RefNumberType> > F;

    	#ifdef GPU
    	std::vector<std::vector<RefNumberType> > GPU_A;
    	std::vector<std::vector<RefNumberType> > GPU_B;
    	std::vector<std::vector<RefNumberType> > GPU_C;
    	#endif

    PageRank_TestFixture1( ) { 
       // initialization code here
       // printf("CONSTRUCT\n");
    } 
    
    // virtual void SetUp( ) { 
    // 	printf("SETUP - default \n");
    // }

    virtual void SetUp( std::string InputFileName ) { 
    	// A = readCSVMatrix<RefNumberType>( std::string("../../DATA/PageRank/All/_abortion/graph/adj_matrix"), ','); 
    	// A = readCSVMatrix<RefNumberType>( std::string("../../DATA/UnitCheckData/float_in000.csv"), ',');
    	A = readCSVMatrix<RefNumberType>( InputFileName, ','); 
    	// printf("SETUP\n");
    	B = A;
    	C = A;
    	D = A;	
    	E = A;
    	F = A;

    	#ifdef GPU
    	GPU_A = A;
    	GPU_B = B;
    	GPU_C = C;
    	#endif
    } 

    virtual void SetUp2( std::string InputFileName ) { 
    	// A = readCSVMatrix<RefNumberType>( std::string("../../DATA/PageRank/All/_abortion/graph/adj_matrix"), ','); 
    	// A = readCSVMatrix<RefNumberType>( std::string("../../DATA/UnitCheckData/float_in000.csv"), ',');
    	A = readCSVMatrix<RefNumberType>( std::string(my_argv[2]) + InputFileName, ','); 
    	// printf("SETUP\n");
    	B = A;
    	C = A;
    	D = A;	
    	E = A;
    	F = A;

    	#ifdef GPU
    	GPU_A = A;
    	GPU_B = A;
    	GPU_C = A;
    	#endif
    }
 
    virtual void TearDown( ) { 
       // code here will be called just after the test completes
       // ok to through exceptions from here if need be
    }
 
    ~PageRank_TestFixture1( )  { 
       // cleanup any pending stuff, but no exceptions allowed
    } 
 
   // put in any custom data members that you need 
};

TEST_P(PageRank_TestFixture1, Test1)
{
    const int Nthr              = 80; 
    RefNumberType const d     	= std::tr1::get<0>(GetParam());
    RefNumberType const eps   	= std::tr1::get<1>(GetParam());
    int DataSource 	 			= std::tr1::get<2>(GetParam());
    std::string InFile  		= std::tr1::get<3>(GetParam());

    SetUp( std::string(my_argv[DataSource]) + InFile );

    double time;
	std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 0, &time, Nthr );
    std::vector<RefNumberType> ret1 = PageRank( B, d, eps, (unsigned) 1, &time, Nthr );
    std::vector<RefNumberType> ret2 = PageRank( C, d, eps, (unsigned) 2, &time, Nthr );

    std::vector<RefNumberType> ret10 = PageRank( D, d, eps, (unsigned) 10, &time, Nthr );
    std::vector<RefNumberType> ret11 = PageRank( E, d, eps, (unsigned) 11, &time, Nthr );
    std::vector<RefNumberType> ret12 = PageRank( F, d, eps, (unsigned) 12, &time, Nthr );

	Vector_FLOAT_EQ<RefNumberType>( ret0, ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, ret2 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret2 );

	Vector_FLOAT_EQ<RefNumberType>( ret0, ret10 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret11 );
	Vector_FLOAT_EQ<RefNumberType>( ret2, ret12 );

    #ifdef GPU
    std::vector<RefNumberType> GPU_ret0 = PageRank( GPU_A, d, eps, (unsigned) 100, &time, Nthr );
	std::vector<RefNumberType> GPU_ret1 = PageRank( GPU_B, d, eps, (unsigned) 101, &time, Nthr );
    std::vector<RefNumberType> GPU_ret2 = PageRank( GPU_C, d, eps, (unsigned) 102, &time, Nthr );

	Vector_FLOAT_EQ<RefNumberType>( ret0, GPU_ret0 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, GPU_ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, GPU_ret2 );

    #endif

}

#ifdef GPU
TEST (GpuTests,  KernelReduction ) { 
	int n = 10;
	RefNumberType* init_vec_1 = new RefNumberType[n];
	RefNumberType* init_vec_2 = new RefNumberType[n];

	for( int i = 0; i < n; ++ i )
	{
		init_vec_1[i] = i;
		init_vec_2[i] = 0;
	}
	
	RefNumberType tmpErr = TestWrapper_KERNEL_ReduceError( init_vec_1, init_vec_2, n, 32);

	delete[] init_vec_1;
	delete[] init_vec_2;

    EXPECT_FLOAT_EQ ( 285, tmpErr ); // 0^2 + 1^2 ... + 9^2. 
} 

TEST (GpuTests,  KernelReduction_2 ) { 
	int n = 10*1000;
	RefNumberType* init_vec_1 = new RefNumberType[n];
	RefNumberType* init_vec_2 = new RefNumberType[n];

	for( int i = 0; i < n; ++ i )
	{
		init_vec_1[i] = i % 10;
		init_vec_2[i] = 0;
	}
	
	RefNumberType tmpErr = TestWrapper_KERNEL_ReduceError( init_vec_1, init_vec_2, n, 32);

	delete[] init_vec_1;
	delete[] init_vec_2;

    EXPECT_FLOAT_EQ ( 285*1000, tmpErr ); // 0^2 + 1^2 ... + 9^2. 
} 

#endif //*/

INSTANTIATE_TEST_CASE_P(
  SweepD_GPU_N5, PageRank_TestFixture1, testing::Values(
    //           		   d 	 eps
    ::testing::make_tuple( 0.9,  1e-14,	1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.85, 1e-14, 1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.5,  1e-14, 1, std::string("/float_in000.csv") ),

    ::testing::make_tuple( 0.9,  1e-14,	1, std::string("/float_in001.csv") ),
    ::testing::make_tuple( 0.85, 1e-14, 1, std::string("/float_in001.csv") ),
    ::testing::make_tuple( 0.5,  1e-14, 1, std::string("/float_in001.csv") ),

    ::testing::make_tuple( 0.9,  1e-14,	1, std::string("/float_in002.csv") ),
    ::testing::make_tuple( 0.85, 1e-14, 1, std::string("/float_in002.csv") ),
    ::testing::make_tuple( 0.5,  1e-14, 1, std::string("/float_in002.csv") )// ...
  ));  

INSTANTIATE_TEST_CASE_P(
  SweepD, PageRank_TestFixture1, testing::Values(
    //           		   d 	 eps
    ::testing::make_tuple( 0.9,  1e-14,	1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.85, 1e-14, 1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.5,  1e-14, 1, std::string("/float_in000.csv") )
    // ...
  ));  

INSTANTIATE_TEST_CASE_P(
  SweepEps, PageRank_TestFixture1, testing::Values(
    //           		   d 	 eps
    ::testing::make_tuple( 0.85, 1e-14,	1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.85, 1e-10, 1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.85, 1e-6, 	1, std::string("/float_in000.csv") ),
    ::testing::make_tuple( 0.85, 1e-4,  1, std::string("/float_in000.csv") )
    // ...
  )); //*/

INSTANTIATE_TEST_CASE_P(
  OneFullInput, PageRank_TestFixture1, testing::Values(
    //           		   d 	 eps
    ::testing::make_tuple( 0.85, 1e-14,	2, std::string("/_abortion/graph/adj_matrix") )
    // ...
  )); 
//*/
/*
INSTANTIATE_TEST_CASE_P(
  ALLFullInput, PageRank_TestFixture1, testing::Values(
    //           		   d 	 eps
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_abortion/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_affirmative_action/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_alcohol/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_amusement_parks/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_architecture/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_armstrong/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_automobile_industries/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_basketball/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_blues/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_cheese/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_classical_guitar/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_complexity/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_computational_complexity/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_computational_geometry/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_death_penalty/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_genetic/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_geometry/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_globalization/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_gun_control/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_iraq_war/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_jaguar/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_jordan/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_moon_landing/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_movies/graph/adj_matrix") ), 

	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_national_parks/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_net_censorship/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_randomized_algorithms/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_recipes/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_roswell/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_search_engines/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_shakespeare/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_table_tennis/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_vintage_cars/graph/adj_matrix") ), 
	::testing::make_tuple( 0.85, 1e-14, std::string("../../localdata/PageRank/_weather/graph/adj_matrix") )
    // ...
  )); 
//*/


/* // Replace that code with parametric tests.
TEST_F (PageRank_TestFixture1, UnitTest1) 
{	
	RefNumberType d    = 0.85;
	RefNumberType eps  = 1e-14;

	std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 0 );
    std::vector<RefNumberType> ret1 = PageRank( B, d, eps, (unsigned) 1 );
    std::vector<RefNumberType> ret2 = PageRank( C, d, eps, (unsigned) 2 );

	Vector_FLOAT_EQ<RefNumberType>( ret0, ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, ret2 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret2 );
}

TEST_F (PageRank_TestFixture1, UnitTest2) 
{	
	RefNumberType d    = 0.85;
	RefNumberType eps  = 1e-6;

	std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 0 );
    std::vector<RefNumberType> ret1 = PageRank( B, d, eps, (unsigned) 1 );
    std::vector<RefNumberType> ret2 = PageRank( C, d, eps, (unsigned) 2 );

	Vector_FLOAT_EQ<RefNumberType>( ret0, ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, ret2 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret2 );
}

TEST_F (PageRank_TestFixture1, UnitTest3) 
{	
	RefNumberType d    = 0.5;
	RefNumberType eps  = 1e-6;

	std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 0 );
    std::vector<RefNumberType> ret1 = PageRank( B, d, eps, (unsigned) 1 );
    std::vector<RefNumberType> ret2 = PageRank( C, d, eps, (unsigned) 2 );

	Vector_FLOAT_EQ<RefNumberType>( ret0, ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, ret2 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret2 );
} //*/


/* 
TEST (PageRank,  ThreeMethods ) {

    std::vector<std::vector<RefNumberType> > A;
    // A = readCSVMatrix<RefNumberType>( std::string("../../DATA/PageRank/All/_abortion/graph/adj_matrix"), ',');
    A = readCSVMatrix<double>( std::string("../../DATA/UnitCheckData/float_in000.csv"), ',');
    std::vector<std::vector<RefNumberType> > B;
    std::vector<std::vector<RefNumberType> > C;
    B = A; 
    C = A;

	size_t l = (10<A.size())? 10: A.size();

	PrintMatrix( A, 0, l, 0, l );
	printf("----------------------------------------------------------------------------------\n");

	RefNumberType d 	= 0.85;
	// RefNumberType eps  = 1e-6; // used to stop for float
	RefNumberType eps  = 1e-14; // used to stop for double

    std::vector<RefNumberType> ret0 = PageRank( A, d, eps, (unsigned) 0 );
    	PrintMatrix( A, 0, l, 0, l );

    std::vector<RefNumberType> ret1 = PageRank( B, d, eps, (unsigned) 1 );
		PrintMatrix( B, 0, l, 0, l );

    std::vector<RefNumberType> ret2 = PageRank( C, d, eps, (unsigned) 2 );
    PrintMatrix( C, 0, l, 0, l );


	Vector_FLOAT_EQ<RefNumberType>( ret0, ret1 );
	Vector_FLOAT_EQ<RefNumberType>( ret0, ret2 );
	Vector_FLOAT_EQ<RefNumberType>( ret1, ret2 );

	std::vector< std::vector<RefNumberType> > RowLine0;
	RowLine0.push_back( ret0 );
	PrintMatrix( RowLine0, 0, 0, 0, l);
	
	std::vector< std::vector<RefNumberType> > RowLine1;
	RowLine1.push_back( ret1 );
	PrintMatrix( RowLine1, 0, 0, 0, l);

	std::vector< std::vector<RefNumberType> > RowLine2;
	RowLine2.push_back( ret2 );
	PrintMatrix( RowLine2, 0, 0, 0, l);
} //*/

int main(int argc, char **argv) {
  	::testing::InitGoogleTest(&argc, argv);
  	my_argc = argc;
    my_argv = argv;

    if( argc != 3 )
    {
        printf("Usage: ./MB001 <GTEST PARAMS> <DIRECTORY TO (SMALL) DATA> <DIR TO PAGERANK DATA>\n");
        exit(1);
    }
  	return RUN_ALL_TESTS();
}


