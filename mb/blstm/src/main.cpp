//
// Copyright (C) 2017 University of Kaiserslautern
// Microelectronic Systems Design Research Group
//
// Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
// 20. February 2017
//



#include "neuron.hpp"
#include <sstream>

int main(int argc, const char* argv[])
{

	// Modifications to allow correct usage.
	if( argc != 4)
	{
		print_usage_and_exit(argv[0]);
	}

	std::string parallelization = argv[1];
	if (parallelization == "-s")
	{
		omp_set_num_threads(1);
	}
	else if (parallelization == "-m")
	{
		omp_set_num_threads(GetNumberOfThreads());
	}
	else
	{
		print_usage_and_exit(argv[0]);
	}
	
	unsigned int numThreads = GetNumberOfThreads();
	std::cout << "Number of threads on the current machine: " << numThreads << std::endl; 
	char hostname[256];
	gethostname(hostname, 256);
	std::cout << "Host name: " << hostname << std::endl;
	
	//----------------------------------------------------------------------
	// Init main data structures
	//----------------------------------------------------------------------	
	
	// Source directories for images and ground truth files 
	std::string inputFileImageDir = argv[2];
	std::string inputFileGroundTruthDir = argv[3];
	//std::string inputFileGroundTruthDir = "../gt/";
		
	// The initialization of the NN for processing image in the forward direction
	NeuralNetwork neuralNetwork_fw;
	neuralNetwork_fw.Init("../model/model_fw.txt");
	
	// The initialization of the NN for processing image in the backward direction
	NeuralNetwork neuralNetwork_bw;
	neuralNetwork_bw.Init("../model/model_bw.txt");	
	
	// The initialization of the alphabet
	Alphabet alphabet;
	alphabet.Init("../alphabet/alphabet.txt");
	
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
			
	return 0;
}

