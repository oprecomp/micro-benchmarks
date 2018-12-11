#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cassert>
#include <random>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"
#include "Check.hpp"
#include "GenzFunctions.hpp"

#include "AmesterMeasurements.hpp"
#include "BenchmarkRunner.hpp"

class myBenchmarkRunner : BenchmarkRunner
{
	protected:
		std::vector<int> 		_Nthrs;
		std::vector<int>		_DataFunctionId;

	public:
		myBenchmarkRunner(std::string OufFilePrefix, int portEncodeUseAMESTER ) : BenchmarkRunner( OufFilePrefix, portEncodeUseAMESTER){}

		void Construct();
		void Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx );
		void RunAll();
};

void myBenchmarkRunner::Construct()
{
	/////////////////////////////////////////////////////////////////////////////////////
	// ALL PARAMETERS USED TO RUN THE KERNEL THAT WILL CHANGE
	/////////////////////////////////////////////////////////////////////////////////////
	// _Nthrs.push_back(1);
	// _Nthrs.push_back(4);
	// _Nthrs.push_back(32);
	// _Nthrs.push_back(128);
	_Nthrs.push_back(512);

 	/////////////////////////////////////////////////////////////////////////////////////
	// ALL DATA SOURCE SELECTION
	/////////////////////////////////////////////////////////////////////////////////////
	_DataFunctionId.push_back(1);
	_DataFunctionId.push_back(2);
	// _DataFunctionId.push_back(3);
	// _DataFunctionId.push_back(4);
	// _DataFunctionId.push_back(5);
	// _DataFunctionId.push_back(6);
}

void myBenchmarkRunner::Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
{
	assert( DataIdx < _DataFunctionId.size() );
	assert( ParamIdx < _Nthrs.size() );

	printf("Run Kernel %i %i %i\n", DataIdx, ParamIdx, RepIdx ); 

	AmesterMeasurements AM(_MeasureModeStr, ConstructOutFileName( DataIdx, ParamIdx, RepIdx ), _host, _port );

	int Nsplits  = 1000;  // Number of sub-intervalls
	int Npoints  = 1024;  // GLI support points
	int Nrep_max = 10;   // number of repetitions with random parameters
    
    // Random generator and seed.
    int seed = 0; 
    std::mt19937 rng(seed);
    // uniform distribution geneator in [0,1]
    // Two independet random streams:
    std::uniform_real_distribution<double> uniform_u(0, 1);
    std::uniform_real_distribution<double> uniform_a(0, 1);

	DataHandle_1D_Type<double> data_1d;

	std::vector<double> results = std::vector<double>(Nrep_max);
	std::vector<double> goldenref = std::vector<double>(Nrep_max);
	std::vector<double> us = std::vector<double>(Nrep_max);
	std::vector<double> as = std::vector<double>(Nrep_max);

	// Generate Data
	for( int Nrep = 0; Nrep < Nrep_max; ++ Nrep )
	{
		us[Nrep] = (double) uniform_u(rng);
		as[Nrep] = (double) uniform_a(rng);
	}

	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT START
	//====================================================================================================================================================================================================================
	AM.start();

	for( int Nrep = 0; Nrep < Nrep_max; ++ Nrep )
	{
    	data_1d.FunctionId 	= _DataFunctionId[DataIdx];
	    data_1d.u     		= us[Nrep];
	    data_1d.a 			= as[Nrep];

	    results[Nrep]		= gauss_legendre_splitEqualIntervalls_OMP( Nsplits, Npoints, f_handle_1d, &data_1d, 0, 1, _Nthrs[ParamIdx] );
	}
	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT STOP
	//====================================================================================================================================================================================================================
	AM.stop();


	// Generate all the reference values
	for( int Nrep = 0; Nrep < Nrep_max; ++ Nrep )
	{
    	data_1d.FunctionId 	= _DataFunctionId[DataIdx];
	    data_1d.u     		= us[Nrep];
	    data_1d.a 			= as[Nrep];

		goldenref[Nrep] = RefIntegeral( data_1d.FunctionId, data_1d.u, data_1d.a );
	}

	writeCSVLine<double>( results, std::string("results_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx ), ';');
	writeCSVLine<double>( goldenref, std::string("goldenref_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx ), ';');
	writeCSVLine<double>( us, std::string("ParamU_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx ), ';');
	writeCSVLine<double>( as, std::string("ParamA_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx ), ';');
}

void myBenchmarkRunner::RunAll()
{
	RunSelection( 	0,	_DataFunctionId.size(),
					0,	_Nthrs.size(),
					0, 	_Nreps );
}

int main(int argc, char **argv) {

	if( argc != 2 )
	{
		printf("Usage: %s <OUT DATA PREFIX>\n", argv[0]);
		exit(1);
	}
	myBenchmarkRunner myBenchmark( argv[1], 0 );
	myBenchmark.Construct();
	myBenchmark.RunAll();
}
