#include <cmath>
#include <stdint.h>
#include <ctime>
#include <chrono>

#include <vector>
#include <string>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"

#include "AmesterMeasurements.hpp"
#include "BenchmarkRunner.hpp"

// 1) Start Amester:
// amester --nogui ServerAmesterMeasurements.tcl scaligera monitor monitor 9999
//
// 2) Run that script 

class myBenchmarkRunner : BenchmarkRunner
{
	protected:
		// ALL CONSTANT PARAMETERS USED TO RUN THE KERNEL
		RefNumberType const 	_d     	= 0.9;
    	RefNumberType const 	_eps   	= 1e-14;

    	// ALL PARAMETERS USED TO RUN THE KERNEL THAT WILL CHANGE
		std::vector<unsigned> 	_modes;
		std::vector<int> 		_Nthrs;

		// ALL DATA SOURCE SELECTION
		std::vector< std::string > _dataInput;

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
	// CPU VERSIONS
	_modes.push_back(0); 	_Nthrs.push_back(-1);
	_modes.push_back(1);	_Nthrs.push_back(-1);
	_modes.push_back(2);	_Nthrs.push_back(-1);

	// OPENMP VERSIONS
	_modes.push_back(10); 	_Nthrs.push_back(1);
	_modes.push_back(10); 	_Nthrs.push_back(4);
	_modes.push_back(10); 	_Nthrs.push_back(32);
	_modes.push_back(10); 	_Nthrs.push_back(128);
	_modes.push_back(10); 	_Nthrs.push_back(512);

	_modes.push_back(11); 	_Nthrs.push_back(1);
	_modes.push_back(11); 	_Nthrs.push_back(4);
	_modes.push_back(11); 	_Nthrs.push_back(32);
	_modes.push_back(11); 	_Nthrs.push_back(128);
	_modes.push_back(11); 	_Nthrs.push_back(512);

	_modes.push_back(12); 	_Nthrs.push_back(1);
	_modes.push_back(12); 	_Nthrs.push_back(4);
	_modes.push_back(12); 	_Nthrs.push_back(32);
	_modes.push_back(12); 	_Nthrs.push_back(128);
	_modes.push_back(12); 	_Nthrs.push_back(512);

	// GPU VERSIONS
	_modes.push_back(100);	  _Nthrs.push_back(-1);
	_modes.push_back(101);	  _Nthrs.push_back(-1);
	_modes.push_back(102);	  _Nthrs.push_back(-1);

	assert( _modes.size() == _Nthrs.size() );

 	/////////////////////////////////////////////////////////////////////////////////////
	// ALL DATA SOURCE SELECTION
	/////////////////////////////////////////////////////////////////////////////////////
	// _dataInput.push_back("/dataL/eid/GIT/oprecomp/mb/pagerank/testsuite/data/float_in000.csv");
	// Do not use the hard-coded data link as in the following:
	//_dataInput.push_back("/dataL/eid/GIT/oprecomp/mb/pagerank/build/data/prepared/mb/pagerank/_abortion/graph/adj_matrix");
	//_dataInput.push_back("/Users/eid/Desktop/GIT/oprecomp/mb/pagerank/build/data/prepared/mb/pagerank/_abortion/graph/adj_matrix");
	// Run this script in the "build" folder where there is the sim-link to the correct place of the data. Make sure you start the script from the correct place.
	_dataInput.push_back("data/prepared/mb/pagerank/_abortion/graph/adj_matrix");
}

std::vector<RefNumberType> PageRank_MEASURE( std::vector<std::vector<RefNumberType> > &A, RefNumberType d, RefNumberType eps, unsigned mode, int Nthr, AmesterMeasurements& AM )
{
	const int MoptinternelReps = 30;
	const int MGPUinternelReps = 50;

	std::vector<RefNumberType> ret;

	if( mode == 0 || mode == 10 )
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );		
		AM.start();
		//----------------------------------------------------------------------
		if( mode == 0) ret = PageRank_Dense( A, d, eps );
		else ret = PageRank_Dense_OMP( A, d, eps, Nthr );
		//----------------------------------------------------------------------
		AM.stop();
	}else if( mode == 1 || mode == 11)
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		AM.start();
		//----------------------------------------------------------------------
		if( mode == 1) ret = PageRank_CSR( S, A.size(), d, eps );
		else ret = PageRank_CSR_OMP( S, A.size(), d, eps, Nthr );
		//----------------------------------------------------------------------
		AM.stop();
	}else if( mode == 2 || mode == 12 )
	{
		std::vector<size_t> MaskLine = NormalizeRows( A );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		AM.start();
		for( int i=0; i<MoptinternelReps; ++i )
		{
			//----------------------------------------------------------------------
			if( mode == 2) ret = PageRank_CSR_OPT( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps  );
			else ret = PageRank_CSR_OPT_OMP( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps, Nthr );
			//----------------------------------------------------------------------
		}
		AM.stop( 1/(double) MoptinternelReps );
	}else if( mode == 100 )
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		#ifdef GPU
			AM.start();
			for( int i=0; i<MGPUinternelReps; ++i )
			{
				//----------------------------------------------------------------------
				ret = GPU_PageRank_Dense( A, d, eps );
				//----------------------------------------------------------------------
			}
			AM.stop( 1/(double) MGPUinternelReps );

			// printf("PRANK call duartion: %.2f ms \n" , (*time) );
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif

	}else if( mode == 101)
	{
		NormalizeRows( A, 1/((double) A.size() ) );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		#ifdef GPU
			AM.start();
			for( int i=0; i<MGPUinternelReps; ++i )
			{
				//----------------------------------------------------------------------
				ret =  GPU_PageRank_CSR( S,  A.size(), d, eps );
				//----------------------------------------------------------------------
			}
			AM.stop( 1/(double) MGPUinternelReps );
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif
	}else if( mode == 102)
	{
		std::vector<size_t> MaskLine = NormalizeRows( A );
		Tp( A );
		CSRType<RefNumberType> S = Dense2Sparse( A );
		#ifdef GPU
			AM.start();
			for( int i=0; i<MGPUinternelReps; ++i )
			{
				//----------------------------------------------------------------------
				ret = GPU_PageRank_CSR_OPT( S, A.size(), MaskLine, 1/((double) A.size() ), d, eps );
				//----------------------------------------------------------------------
			}
			AM.stop( 1/(double) MGPUinternelReps );
		#else 
			printf("The code runs with a GPU mode = %u\n but was compiled WHITOUTH GPU support!\n(turn on the GPU flag with -DGPU during compilation)\n", mode);
			exit(-1);
		#endif
	} else
	{
		printf("mode = %u not supported\n", mode);
		exit(-1);
	}

	return ret;
}

void myBenchmarkRunner::Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
{
	assert( DataIdx < _dataInput.size() );
	assert( ParamIdx < _modes.size() );

	AmesterMeasurements M(_MeasureModeStr, ConstructOutFileName( DataIdx, ParamIdx, RepIdx ), _host, _port );

	// Preparation (not measured)
	//----------------------------------------------------------------------
	std::vector<std::vector<RefNumberType> > A = readCSVMatrix<RefNumberType>( _dataInput[ DataIdx ], ','); 
	
	std::vector<RefNumberType> ret = PageRank_MEASURE( A, _d, _eps, _modes[ ParamIdx ], _Nthrs[ ParamIdx ], M );

	writeCSVLine<RefNumberType>( ret, std::string("NodeScore_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx ), ';');

}

void myBenchmarkRunner::RunAll()
{
	RunSelection( 	0,	_dataInput.size(),
					0,	_modes.size(),
					0, 	_Nreps );
}


// call with: /fl/eid/DATA/Out002/TmpOut_
int main(int argc, char **argv) {
	if( argc != 2 )
	{
		printf("Usage: %s <OUT DATA PREFIX>\n", argv[0]);
		exit(1);
	}
	myBenchmarkRunner myBenchmark(argv[1], 0);
	myBenchmark.Construct();
	myBenchmark.RunAll();
}