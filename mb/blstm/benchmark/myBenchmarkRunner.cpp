#include <cmath>
#include <stdint.h>
#include <ctime>
#include <chrono>

#include <vector>
#include <string>

#include "AmesterMeasurements.hpp"
#include "BenchmarkRunner.hpp"

#include "neuron.hpp"
#include <sstream>

// 1) Start Amester:
// amester --nogui ServerAmesterMeasurements.tcl scaligera monitor monitor 9999
//
// 2) Run that script 

class myBenchmarkRunner : BenchmarkRunner
{
	protected:
		std::vector<int> 		_Nthrs;
		std::string _inputFileImageDir;
		std::string _inputFileGroundTruthDir;
		std::string _SourceRootDir;

	public:
		myBenchmarkRunner(std::string OufFilePrefix, int portEncodeUseAMESTER, std::string inputFileImageDir, std::string inputFileGroundTruthDir, std::string SourceRootDir) : BenchmarkRunner( OufFilePrefix, portEncodeUseAMESTER)
		{
			_inputFileImageDir = inputFileImageDir;
			_inputFileGroundTruthDir = inputFileGroundTruthDir;
			_SourceRootDir = SourceRootDir;
		}

		void Construct();
		void Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx );
		void RunAll();

};

void myBenchmarkRunner::Construct()
{
	/////////////////////////////////////////////////////////////////////////////////////
	// ALL PARAMETERS USED TO RUN THE KERNEL THAT WILL CHANGE
	/////////////////////////////////////////////////////////////////////////////////////
	_Nthrs.push_back(1);
	_Nthrs.push_back(4);
	_Nthrs.push_back(32);
	_Nthrs.push_back(128);
	_Nthrs.push_back(512);

 	/////////////////////////////////////////////////////////////////////////////////////
	// ALL DATA SOURCE SELECTION
	/////////////////////////////////////////////////////////////////////////////////////
	// Non. Input is always the full old german (Fraktur) dataset.
}

// The following code is the modified main.cpp of the BLSTM benchmark, instrumented to perform
// power measurements on relevant kernels.
double BLSTM_main_MEASURE( int Nthr, AmesterMeasurements& AM, std::string inputFileImageDir, std::string inputFileGroundTruthDir, std::string SourceRootDir )
{
	// omp_set_num_threads(GetNumberOfThreads());
	omp_set_num_threads( Nthr );

	unsigned int numThreads = GetNumberOfThreads();
	std::cout << "Number of threads on the current machine: " << numThreads << std::endl; 
	char hostname[256];
	gethostname(hostname, 256);
	std::cout << "Host name: " << hostname << std::endl;
	
	//----------------------------------------------------------------------
	// Init main data structures
	//----------------------------------------------------------------------	
	
	// Source directories for images and ground truth files 
	// std::string inputFileImageDir = argv[2];
	// std::string inputFileGroundTruthDir = argv[3];
	//std::string inputFileGroundTruthDir = "../gt/";
	// Checking Paths:
	//std::cout << "\"" << SourceRootDir + "/model/model_fw.txt" << "\"\n";
	//std::cout << "\"" << inputFileImageDir << "\"\n";		

	// The initialization of the NN for processing image in the forward direction
	NeuralNetwork neuralNetwork_fw;
	neuralNetwork_fw.Init(SourceRootDir + "/model/model_fw.txt");
	

	// The initialization of the NN for processing image in the backward direction
	NeuralNetwork neuralNetwork_bw;
	neuralNetwork_bw.Init(SourceRootDir + "/model/model_bw.txt");	
	
	// The initialization of the alphabet
	Alphabet alphabet;
	alphabet.Init(SourceRootDir + "/alphabet/alphabet.txt");
	
	// Return the list of images' file names
	std::vector<std::string> listOfImages = open(inputFileImageDir);
	
	// Return the list of ground truth' file names
	std::vector<std::string> listOfGroundTruth = open(inputFileGroundTruthDir);
		
	//----------------------------------------------------------------------
	// Read Images
	//----------------------------------------------------------------------
	
	std::vector<InputImage> vecInputImage;
	vecInputImage.resize(listOfImages.size());
	
	#pragma omp parallel
	#pragma omp for schedule(dynamic)
	for(unsigned int i = 0; i < listOfImages.size(); i++)
	{
		std::string inputFileImage = inputFileImageDir + listOfImages.at(i);
		
		vecInputImage.at(i).Init(inputFileImage);
	}
	
	//----------------------------------------------------------------------
	// Read Ground Truth
	//----------------------------------------------------------------------
	
	std::vector<GroundTruth> vecGroundTruth;
	vecGroundTruth.resize(listOfImages.size());
	
	#pragma omp parallel
	#pragma omp for schedule(dynamic)
	for(unsigned int i = 0; i < listOfImages.size(); i++)
	{
		std::string inputFileGroundTruth = inputFileGroundTruthDir + listOfGroundTruth.at(i);
		
		vecGroundTruth.at(i).Init(inputFileGroundTruth);
	}
	
	//----------------------------------------------------------------------
	// Predicted string
	//----------------------------------------------------------------------
	
	std::vector<std::string> vecPredictedString;
	vecPredictedString.resize(listOfImages.size());

	//----------------------------------------------------------------------
	// Allocation of the resources
	//----------------------------------------------------------------------
	
	// Output of the Hidden Layer after processing Image in Forward direction
	float *outputFromtHiddenLayer_fw = new float [numThreads * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_NEURONS];	 
	// Output of the Hidden Layer after processing Image in Backward direction
	float *outputFromtHiddenLayer_bw = new float [numThreads * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_NEURONS]; 	
	// Output of the Softmax Layer - the Outout Layer
	float *outputFromOutputLayer = new float [numThreads * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_CLASSES];
	
	double *error = new double [listOfImages.size()];	
	double errorSum = 0.0;
	double accuracy = 0.0;
	
	//====================================================================================================================================================================================================================
	// START
	//====================================================================================================================================================================================================================
	
	std::cout << "Start ..." << std::endl;

	// Starting point - the first time stemp		
	auto t1 = std::chrono::high_resolution_clock::now();

	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT START
	//====================================================================================================================================================================================================================
	AM.start();
			
	#pragma omp parallel
	#pragma omp for schedule(dynamic)
	for(unsigned int i = 0; i < vecInputImage.size(); i++)
	{	
		unsigned int threadID = omp_get_thread_num();
		
		float *pOutputFromtHiddenLayer_fw = outputFromtHiddenLayer_fw + threadID * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_NEURONS;
		float *pOutputFromtHiddenLayer_bw = outputFromtHiddenLayer_bw + threadID * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_NEURONS;		
		float *poutputFromOutputLayer   = outputFromOutputLayer   + threadID * MAX_NUMBER_COLUMNS_TEST_SET * NUMBER_OF_CLASSES;			
		
		const unsigned int numberOfColumns = vecInputImage.at(i).numberOfColumns;			 
				
		// Forward direction
		Hidden_Layer(vecInputImage.at(i).image_fw,			
					 numberOfColumns,				 
					 neuralNetwork_fw.WGI, 					
					 neuralNetwork_fw.WGF, 					
					 neuralNetwork_fw.WGO, 					
					 neuralNetwork_fw.WCI, 					
					 neuralNetwork_fw.WIP, 					
					 neuralNetwork_fw.WFP, 					
					 neuralNetwork_fw.WOP,					
					 pOutputFromtHiddenLayer_fw);
		
		// Backward direction
		Hidden_Layer(vecInputImage.at(i).image_bw,				
					 numberOfColumns,				 
					 neuralNetwork_bw.WGI, 					
					 neuralNetwork_bw.WGF, 					
					 neuralNetwork_bw.WGO, 					
					 neuralNetwork_bw.WCI, 					
					 neuralNetwork_bw.WIP, 					
					 neuralNetwork_bw.WFP, 					
					 neuralNetwork_bw.WOP,					
					 pOutputFromtHiddenLayer_bw);
								
		//inputImage.Free();							
		
		// CTC - Output Layer
		Output_Layer(numberOfColumns,
					 neuralNetwork_fw.W2, 					
					 pOutputFromtHiddenLayer_fw, 	
					 pOutputFromtHiddenLayer_bw,	
					 poutputFromOutputLayer);				
				
		// Return the predicted string
		TranslateBack(alphabet, numberOfColumns, poutputFromOutputLayer, vecPredictedString.at(i));					
	}
	
	//====================================================================================================================================================================================================================
	// OPRECOMP MEASUREMENT STOP
	//====================================================================================================================================================================================================================
	AM.stop();

	// Ending point - the final time stemp	
	auto t2 = std::chrono::high_resolution_clock::now(); 
	
	//====================================================================================================================================================================================================================
	// FINISH
	//====================================================================================================================================================================================================================
	
	#pragma omp parallel
	#pragma omp for schedule(dynamic)
	for(unsigned int i = 0; i < vecInputImage.size(); i++)
	{
				
		//----------------------------------------------------------------------
		// Calculate Levenshtein Distance for each string and output result
		//----------------------------------------------------------------------
		std::string groundTruthstring = vecGroundTruth.at(i).ReturnString();			
		error[i] = LevenshteinDistance(vecPredictedString.at(i), groundTruthstring);	
		//error[i] = LevenshteinDistanceCStyle(predictedString.c_str(), groundTruthstring.c_str());		
		
		#if PROFILE
			std::cout << vecPredictedString.at(i) << std::endl;
			std::cout << groundTruthstring << std::endl << std::endl;
		#endif						
		
		vecPredictedString.at(i).clear();
		groundTruthstring.clear();
		//groundTruth.Free();		
	}	
	
	#pragma omp parallel for reduction(+:errorSum)
	for(unsigned int e = 0; e < listOfImages.size(); e++)
		errorSum += error[e];	
	
	accuracy = (1.0 - errorSum / (float)listOfImages.size()) * 100.0;
	std::cout << "Accuracy: " <<  accuracy << "%" << std::endl;

	errorSum = 0.0;			
		
	std::chrono::seconds time_span = std::chrono::duration_cast<std::chrono::seconds>(t2-t1);	
	std::cout << "Measured time ... " << time_span.count() << " seconds" << std::endl << std::endl;	
	
	//----------------------------------------------------------------------
	// END
	//----------------------------------------------------------------------				
		
	delete[] outputFromtHiddenLayer_bw;			
	delete[] outputFromtHiddenLayer_fw;	
	delete[] outputFromOutputLayer;
	
	delete[] error;
			
	return accuracy;
}

void myBenchmarkRunner::Run( unsigned DataIdx, unsigned ParamIdx, unsigned RepIdx )
{
	AmesterMeasurements M(_MeasureModeStr, ConstructOutFileName( DataIdx, ParamIdx, RepIdx ), _host, _port );
	double accuracy = BLSTM_main_MEASURE(  _Nthrs[ ParamIdx ], M, _inputFileImageDir, _inputFileGroundTruthDir, _SourceRootDir );

	//----------------------------------------------------------------------
	// Write file with accuracy.
	//----------------------------------------------------------------------	
	std::string fileName = std::string("AccuracyResult_") +  ConstructOutFileNameNumberPart( DataIdx, ParamIdx, RepIdx );
	printf("Writing File: %s ... ", fileName.c_str());
	FILE *fp = fopen( fileName.c_str(), "w");
	if( fp == 0 )
	{
		printf("FileStream Error\n");
		exit(-1);
	}
	fprintf(fp, "%.4f\n", accuracy );
	fclose(fp);

}

void myBenchmarkRunner::RunAll()
{
	RunSelection( 	0,	1,
					0,	_Nthrs.size(),
					0, 	_Nreps );
}

int main(int argc, char **argv) {
	if( argc != 5 )
	{
		printf("Usage: %s <OUT DATA PREFIX> <DATA INPUT TEST SET> <DATA INPUT GT> <DATA SRC ROOT>\n", argv[0]);
		printf(" EXAMPLE: %s TmpOut data/prepared/mb/blstm/fraktur_dataset/samples/ data/prepared/mb/blstm/gt/ ..\n", argv[0]);
		exit(1);
	}
	myBenchmarkRunner myBenchmark(argv[1], 0, argv[2], argv[3], argv[4]);
	myBenchmark.Construct();
	myBenchmark.RunAll();
}
