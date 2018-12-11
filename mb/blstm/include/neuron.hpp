//
// Copyright (C) 2017 University of Kaiserslautern
// Microelectronic Systems Design Research Group
//
// Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
// 20. February 2017
//



#ifndef NEURON_HPP
#define NEURON_HPP

#include <string>       // std::string
#include <iostream>     // std::cout, std::cerr
#include <fstream>      // std::ifstream std::ofstream
#include <vector>
#include <math.h>       // tanh, log
#include <dirent.h>
#include <sys/types.h>
#include <algorithm>	// std::sort
#include <ctype.h>		// isspace()
#include <chrono>       // std::chrono::seconds, std::chrono::duration_cast

#include <cstdint>		// std::memcpy()
#include <cstring>		// std::memcpy()

#include <omp.h>

#include <unistd.h>		// gethostname()


#define NUMBER_OF_INPUTS 126
#define NUMBER_OF_NEURONS 100
#define HIGHT_IN_PIX 25
#define NUMBER_OF_CLASSES 110
#define MAX_NUMBER_COLUMNS_TEST_SET 732

#define PROFILE 0

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
	
	//====================================================================================================================================================================================================================
	// NEURAL NETWORK - MODEL
	//====================================================================================================================================================================================================================
	
	class NeuralNetwork
	{
		public:
		
		NeuralNetwork();
		~NeuralNetwork();
		
		void Init(std::string inputFileModel);
						
		// NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
		float *WGI;// Input gate
		float *WGF;// Forget gate
		float *WGO;// Output gate
		float *WCI;// Input to Memory Cell
		
		// NUMBER_OF_NEURONS
		float *WIP;// Input gate
		float *WFP;// Forget gate
		float *WOP;// Output gate		
		
		// NUMBER_OF_CLASSES * (1 + NUMBER_OF_NEURONS * 2)
		float *W2; //Output layer
		
		protected:		
		
		private:	
	};
	
	
	
	
	//====================================================================================================================================================================================================================
	// ALPHABET
	//====================================================================================================================================================================================================================

	class Alphabet
	{
		public:
		
		Alphabet();
		~Alphabet();
		
		// NUMBER_OF_CLASSES
		std::vector<std::string> alphabet;
		
		void Init(std::string inputFileAlphabet);
		
		std::string ReturnSymbol(unsigned int lable);
		
		void Print();
					
		protected:		
		
		private:	
	};
	
	
	
	
	//====================================================================================================================================================================================================================
	// INPUT IMAGE
	//====================================================================================================================================================================================================================

	class InputImage
	{
		public:
		
		InputImage();
		~InputImage();
		
		// NumberOfColumns * HIGHT_IN_PIX
		float *image_fw;
		float *image_bw;
		
		unsigned int numberOfColumns;
		
		void Init(std::string inputFileImage);
		
		void Print();
		
		void Free();
			
		protected:		
		
		private:	
	};
	
	
	
	
	//====================================================================================================================================================================================================================
	// GROUND TRUTH
	//====================================================================================================================================================================================================================
	
	class GroundTruth
	{
		public:
		
		GroundTruth();
		~GroundTruth();
		
		// NUMBER_OF_CLASSES
		std::string groundTruth;
		
		void Init(std::string inputFileGroundTruth);
		
		std::string ReturnString();
		
		void Free();
					
		protected:		
		
		private:	
	};
	
	
	
	
	//====================================================================================================================================================================================================================
	// LSTM
	//====================================================================================================================================================================================================================
	
	// The dot product corresponding to a single gate of the LSTM memory cell
	inline float DotVectorToVector126(float *source, 	// IN  // size: 1.0 + HIGHT_IN_PIX + NUMBER_OF_NEURONS = NUMBER_OF_INPUTS
									  float *weights);	// IN  // size: NUMBER_OF_INPUTS
		
	// The function of a single LSTM memory cell
	void HiddenLayerSingleMemoryCell(float *source,					// IN  // size: 1.0 + HIGHT_IN_PIX + NUMBER_OF_NEURONS = NUMBER_OF_INPUTS
									 unsigned int currentColumn,	// IN  // The current column of the image
									 float in_state,				// IN  // A single input state
									 float *WGI,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WGF,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WGO,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WCI,   					// IN  // size: NUMBER_OF_INPUTS
									 float WIP,						// IN  // A single peephole weight
									 float WFP,						// IN  // A single peephole weight
									 float WOP,						// IN  // A single peephole weight
									 float *out_state,				// OUT // A single output state 
									 float *output);              	// OUT // A single output	
			
	
	void Hidden_Layer(float *image,					// IN  // size: numberOfColumns * HIGHT_IN_PIX
					  unsigned int numberOfColumns,	// IN  //				 
					  float *WGI, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WGF, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WGO, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WCI, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WIP, 					// IN  // size: NUMBER_OF_NEURONS
					  float *WFP, 					// IN  // size: NUMBER_OF_NEURONS
					  float *WOP,					// IN  // size: NUMBER_OF_NEURONS
					  float *result);				// OUT // size: numberOfColumns * NUMBER_OF_NEURONS
	
	
	
	//====================================================================================================================================================================================================================
	// Connectionist Temporal Classification Layer (CTC layer)
	//====================================================================================================================================================================================================================			  
					  
	// The dot product corresponding to a single neuron of the output layer operating on an concatinated output from the forward and the bakward hidden layers
	inline float DotVectorToVector201(float *W2,		// IN  // size: NUMBER_OF_NEURONS * 2 + 1
									  float *input_fw, 	// IN  // size: NUMBER_OF_NEURONS
							          float *input_bw);	// IN  // size: NUMBER_OF_NEURONS
	
	// The function of a single neuron of the output layer
	inline void OutputLayerSinlgleNeuron(float *W2, 		// IN  // size: NUMBER_OF_NEURONS * 2 + 1
										 float *input_fw,	// IN  // size: NUMBER_OF_NEURONS
										 float *input_bw, 	// IN  // size: NUMBER_OF_NEURONS
										 float *output);	// OUT // 	
	
	void Output_Layer(unsigned int numberOfColumns, // IN  //
					  float *W2, 					// IN  // size: NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1)
					  float *input_fw,			 	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  float *input_bw,			 	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  float *output); 				// OUT // size: numberOfColumns * NUMBER_OF_CLASSE
	
	// Reconstruct a line from the labels
	void TranslateBack(Alphabet &alphabet, unsigned int numberOfColumns, float *input, std::string &output, float threshold = 0.7);
	
	double LevenshteinDistance(const std::string& s1, const std::string& s2);		
	double LevenshteinDistanceCStyle(const char *s1, const char *s2);
		
				
	//====================================================================================================================================================================================================================
	// AUXILIARY
	//====================================================================================================================================================================================================================	
		
	std::vector<std::string> open(std::string path);
	
	unsigned int GetNumberOfThreads();
	
	void print_usage_and_exit(const char *argv0);
	
	
#endif
