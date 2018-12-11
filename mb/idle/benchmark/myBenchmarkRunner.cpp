#include <string>
#include <ctime>
#include <chrono>

#include "AmesterMeasurements.hpp"
#include "BenchmarkRunner.hpp"


// 1) Start Amester:
// amester --nogui ServerAmesterMeasurements.tcl scaligera monitor monitor 9999
//
// 2) Run that script 

class myBenchmarkRunner : BenchmarkRunner
{
	protected:
		std::vector<int> 		_Wait;
		std::string _inputFileImageDir;
		std::string _inputFileGroundTruthDir;

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
	_Wait.push_back(10);
	_Wait.push_back(100);
	_Wait.push_back(1000);
	_Wait.push_back(10000);
}

void Wait_MEASURE( int wait,  AmesterMeasurements& AM )
{
	int internelReps = 1;
	// for to short tests (repeat them several times to get accurate power figures.)
	if( wait <= 5000 )
	{
		internelReps = (int) ((5000 / (double) wait) + 0.5);
	}

	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT START
	//====================================================================================================================================================================================================================
	AM.start();

	for( int i=0; i<internelReps; ++i )
	{
		// simply wait (to measure idle power consumption of the system)
		usleep( wait*1000 );
	}
	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT STOP
	//====================================================================================================================================================================================================================
	AM.stop( 1/(double) internelReps );	
}

void myBenchmarkRunner::Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
{
	AmesterMeasurements M(_MeasureModeStr, ConstructOutFileName( DataIdx, ParamIdx, RepIdx ), _host, _port );
	Wait_MEASURE(  _Wait[ ParamIdx ], M );


}

void myBenchmarkRunner::RunAll()
{
	RunSelection( 	0,	1,
					0,	_Wait.size(),
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
