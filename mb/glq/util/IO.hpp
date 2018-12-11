#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

/**
    Reads a comma separated csv file into a matrix.

    @param std::string The name of the file.
    @param char The separator used, for exmple ',' or ';' are typical choices.

    @return The matrix, as nested vectors (outer vector holds the rows, the inner vectors the lines)
*/

template<typename T>
std::vector<std::vector<T> >  InitZeros( int Ndim1, int Ndim2 )
{
	std::vector<std::vector<T> > ret;
	for( int i = 0; i<Ndim1; ++i )
	{
		ret.push_back( std::vector<T>(Ndim2, 0) );
	}
	return ret;
}

template<typename T>
std::vector<std::vector<T> >  InitZeros( int Ndim )
{
	return InitZeros<T>( Ndim, Ndim );
}

template<typename T>
std::vector<T> readCSVHeaderLine( const std::string fileName, const char separator )
{
	printf("Reading File: %s ... ", fileName.c_str());
	std::ifstream input( fileName.c_str() );
	if( !input.good() )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	std::vector<T> ret;

	std::string tmpLine;
	std::string cell;
	size_t lineCnt 	= 0;
	size_t width 	= 0;
	size_t cellCnt 	= 0;

	while( std::getline( input, tmpLine ))
	{
		if( lineCnt == 0 )
		{
			// estimate the number of valid entries in a row by counting the separator occurence
			width = ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1;
		}else
		{
			assert( width == ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1 );
		}

		std::istringstream iss(tmpLine);

		ret.resize( width );

		cellCnt = 0;
		while( std::getline( iss, cell, separator ))
		{
			//ret[lineCnt][cellCnt] = (T) atof( cell.c_str() );
			// printf("Reading value: %f\n", ret[lineCnt][cellCnt]);
			ret[cellCnt] = (T) cell;
			cellCnt++;
		}
		lineCnt++;
	}
	printf("Length = %lu  [OK]\n", ret.size() );
	return ret;
}

template<typename T>
std::vector<T> readCSVLine( const std::string fileName, const char separator )
{
	printf("Reading File: %s ... ", fileName.c_str());
	std::ifstream input( fileName.c_str() );
	if( !input.good() )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	std::vector<T> ret;

	std::string tmpLine;
	std::string cell;
	size_t lineCnt 	= 0;
	size_t width 	= 0;
	size_t cellCnt 	= 0;

	while( std::getline( input, tmpLine ))
	{
		if( lineCnt == 0 )
		{
			// estimate the number of valid entries in a row by counting the separator occurence
			width = ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1;
		}else
		{
			assert( width == ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1 );
		}

		std::istringstream iss(tmpLine);

		ret.resize( width );

		cellCnt = 0;
		while( std::getline( iss, cell, separator ))
		{
			ret[cellCnt] = (T) atof( cell.c_str() );
			cellCnt++;
		}
		lineCnt++;
	}
	printf("Length = %lu  [OK]\n", ret.size() );
	return ret;
}

template<typename T>
std::vector<std::vector<T> > readCSVMatrix( const std::string fileName, const char separator )
{
	printf("Reading File: %s ... ", fileName.c_str());
	std::ifstream input( fileName.c_str() );
	if( !input.good() )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	std::vector<std::vector<T> > ret;

	std::string tmpLine;
	std::string cell;
	size_t lineCnt 	= 0;
	size_t width 	= 0;
	size_t cellCnt 	= 0;

	while( std::getline( input, tmpLine ))
	{
		if( lineCnt == 0 )
		{
			// estimate the number of valid entries in a row by counting the separator occurence
			width = ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1;
		}else
		{
			assert( width == ((size_t) std::count( tmpLine.begin(), tmpLine.end(), separator )) + 1 );
		}

		std::istringstream iss(tmpLine);

		ret.push_back( std::vector<T>(width, 0) );

		cellCnt = 0;
		while( std::getline( iss, cell, separator ))
		{
			ret[lineCnt][cellCnt] = (T) atof( cell.c_str() );
			// printf("Reading value: %f\n", ret[lineCnt][cellCnt]);
			cellCnt++;
		}
		lineCnt++;
	}
	printf("Dimension = %lu x %lu \t [OK]\n", ret.size(), ret[0].size() );
	return ret;
}

/**
    Writes a comma separated csv file.

    @param std::vector< std::vector<T> > & The date to be written.
    @param std::string The name of the file.
    @param char The separator used, for exmple ',' or ';' are typical choices.
*/

template<typename T>
void writeCSVMatrix( const std::vector< std::vector<T> > &A, const std::string fileName, const char separator )
{
	printf("Writing File: %s ... ", fileName.c_str());
	FILE *fp = fopen( fileName.c_str(), "w");
	if( fp == 0 )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	for( size_t i=0; i<A.size(); ++i )
	{
		for( size_t j=0; j<A[i].size(); ++j )
		{
			if( j < A[i].size()-1 )
			{
				fprintf(fp, "%.20f%c", (double) A[i][j], separator);
			}else
			{
				fprintf(fp, "%.20f\n", (double) A[i][j] );
			}
		}
	}

	fclose(fp);

	printf("Dimension = %lu x %lu \t [OK]\n", A.size(), A[0].size() );
}


template<typename T>
void writeCSVLine( const std::vector<T> &A, const std::string fileName, const char separator )
{
	printf("Writing File: %s ... ", fileName.c_str());
	FILE *fp = fopen( fileName.c_str(), "w");
	if( fp == 0 )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	for( size_t j=0; j<A.size(); ++j )
	{
		if( j < A.size()-1 )
		{
			fprintf(fp, "%.20f%c", (double) A[j], separator);
		}else
		{
			fprintf(fp, "%.20f\n", (double) A[j] );
		}
	}

	fclose(fp);

	printf("Dimension = %lu  [OK]\n", A.size() );
}