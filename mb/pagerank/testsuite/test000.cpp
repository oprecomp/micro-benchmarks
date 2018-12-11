#include <cmath>
#include <gtest/gtest.h>


double foo( double x )
{
	if( x >= 0) return sqrt( x );
	else return -1;
} 

TEST (SquareRootTest, PositiveNos) { 
    EXPECT_EQ (18.0, foo(324.0));
    EXPECT_EQ (25.4, foo(645.16));
    EXPECT_EQ (50.332, foo(2533.310224));
}
 
TEST (SquareRootTest, ZeroAndNegativeNos) { 
    ASSERT_EQ (0.0, foo(0.0));
    ASSERT_EQ (-1, foo(-22.0));
}

TEST (FloatTest, BasicAssertions) { 
	// EXPECT_FLOAT_EQ (1/(float)3, 0.3333 );   		// FAIL
	// EXPECT_FLOAT_EQ (1/(float)3, 0.333333 ); 		// FAIL 	
	EXPECT_FLOAT_EQ (1/(float)3, 0.3333333333333 ); 		
	EXPECT_FLOAT_EQ (1/(float)3, 0.33333333333333333333333333333 );  
}

TEST (DoubleTest, BasicAssertions) {  
	// EXPECT_FLOAT_EQ (1/(double)3, 0.3333 ); 			// FAIL
	// EXPECT_FLOAT_EQ (1/(double)3, 0.333333 ); 		// FAIL
	EXPECT_FLOAT_EQ (1/(double)3, 0.3333333333333 );
	EXPECT_FLOAT_EQ (1/(double)3, 0.33333333333333333333333333333 );  
}

TEST ( NearDoubleTest, BasicAssertions) {  			    // ALL OK 
	EXPECT_NEAR (1/(double)3, 0.3333, 1e-4 );
	EXPECT_NEAR (1/(double)3, 0.333333, 1e-6 );
	EXPECT_NEAR (1/(double)3, 0.3333333333333, 1e-10 );
	EXPECT_NEAR (1/(double)3, 0.33333333333333333333333333333, 1e-20 ); 
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}