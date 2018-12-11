#include <cmath>
#include <stdint.h>
#include <ctime>
#include <chrono>

#include <vector>
#include <string>
#include <cassert>

#include "AmesterMeasurements.hpp"

class BenchmarkRunner
{
	protected:
		// Number of repetitions
		const unsigned _Nreps = 3;
		const unsigned _waitBetweenTests = 2000; // ms (here 2 sec)

		// Amester Client Settings (to connect to HOST)
		const std::string _host = "scaligera";

		// Use 9999 as defualt port, Use -1 for debugging and on non POWER8 nodes (no amester session required)
		int _port = 9999;
		const std::string _MeasureModeStr = "BASIC_PWR:ACC";

		std::string _OufFilePrefix;

	public:

		BenchmarkRunner( std::string OufFilePrefix, int portEncodeUseAMESTER )
		{
			_OufFilePrefix = OufFilePrefix;

			if( portEncodeUseAMESTER== -1 ) _port = -1; // DO NOT USE AMESTER (DUMMY RUN ON NON POWER8 MACHINES )
			//if( portEncodeUseAMESTER== 0 );  // USE DEFAULT
			if( portEncodeUseAMESTER > 0 )  _port = portEncodeUseAMESTER; // USE THE GIVEN VALUE AS PORT NUMBER. sw
		}

		std::string ConstructOutFileNameNumberPart( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
		{
			return std::to_string(DataIdx) + "_" + std::to_string(ParamIdx) + "_" + std::to_string(RepIdx);
		}

		std::string ConstructOutFileName( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
		{
			return _OufFilePrefix + ConstructOutFileNameNumberPart(DataIdx, ParamIdx, RepIdx);
		}

		void RunSelection( 	unsigned DataIdx_low, 	unsigned DataIdx_high, 
							unsigned ParamIdx_low, 	unsigned ParamIdx_high,
							unsigned RepIdx_low, 	unsigned RepIdx_high );

		virtual void Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx ) = 0 ;
		virtual void RunAll() = 0;
};

void BenchmarkRunner::RunSelection( unsigned DataIdx_low, 	unsigned DataIdx_high, 
									unsigned ParamIdx_low, 	unsigned ParamIdx_high,
									unsigned RepIdx_low, 	unsigned RepIdx_high )
{
	assert( DataIdx_low 	< DataIdx_high );
	assert( ParamIdx_low 	< ParamIdx_high );
	assert( RepIdx_low 		< RepIdx_high );
	
	std::string fileName = _OufFilePrefix + "config.txt";
	printf("Writing File: %s ... ", fileName.c_str());
	FILE *fp = fopen( fileName.c_str(), "w");
	if( fp == 0 )
	{
		printf("FileStream Error\n");
		exit(-1);
	}
	fprintf(fp, "%i %i %i %i %i %i\n", DataIdx_low, DataIdx_high, 	ParamIdx_low, ParamIdx_high, 	RepIdx_low, RepIdx_high);
	fclose(fp);

	for( unsigned rep = RepIdx_low; rep < RepIdx_high; ++rep ) // Rep Loop
	{
		for( unsigned dIdx = DataIdx_low; dIdx < DataIdx_high; ++dIdx ) // Data Loop
		{
			for( unsigned pIdx = ParamIdx_low; pIdx < ParamIdx_high; ++pIdx ) // Param Loop
			{
				Run( dIdx, pIdx, rep );
				usleep( _waitBetweenTests*1000 );
			}
		}
	}
}
