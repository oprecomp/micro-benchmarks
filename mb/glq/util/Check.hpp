#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <gtest/gtest.h>

template<typename T>
bool consistentRowLength( std::vector<std::vector<T> > &a )
{
	size_t n = a.size();
	size_t rowLength = a[0].size();

	for( size_t i = 0; i<n; ++i )
	{
		if( rowLength != a[i].size() )
			return false;
	}
	return true;
}

template<typename T>
bool sameDim(  std::vector<std::vector<T> > &a, std::vector<std::vector<T> > &b )
{
	if(!consistentRowLength(a)) return false;
	if(!consistentRowLength(b)) return false;

	return (a.size() == b.size()) &&  (a[0].size() == b[0].size());

}

template<typename T>
void Vector_FLOAT_EQ( std::vector<T> const &a, std::vector<T> const &b )
{
	EXPECT_EQ( a.size(), b.size() );

	for( int i = 0; i<a.size(); ++i )
	{
		EXPECT_FLOAT_EQ( a[i], b[i]);
	}
}

template<typename T>
void Matrix_FLOAT_EQ( std::vector<std::vector<T> > const &a, std::vector<std::vector<T> > const &b )
{
	EXPECT_EQ( consistentRowLength( a ), true );
	EXPECT_EQ( consistentRowLength( b ), true );
	EXPECT_EQ( a.size(), b.size() );
	EXPECT_EQ( a[0].size(), b[0].size() );

	for( int i = 0; i<a.size(); ++i )
	{
		for( int j = 0; j<a[0].size(); ++j )
		{
			EXPECT_FLOAT_EQ( a[i][j], b[i][j]);
		}

	}
}