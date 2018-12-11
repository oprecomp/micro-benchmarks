// Copyright (c) 2017 OPRECOMP Project
//
// Original BLSTM code Copyright (C) 2017 University of Kaiserslautern
// Microelectronic Systems Design Research Group
// Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
// 20. February 2017
//
// Ported to PULP by Dionysios Diamantopoulos <did@zurich.ibm.com>
// December 2017


// for checking provided directory exists
#include <sys/stat.h>

#include "neuron.hpp"

	//====================================================================================================================================================================================================================
	// NEURAL NETWORK - MODEL
	//====================================================================================================================================================================================================================

	// Constructor
	NeuralNetwork::NeuralNetwork()
	{
		// Hidden layer weigths
		WGI = new float [NUMBER_OF_NEURONS * NUMBER_OF_INPUTS];// Input Gate
		WGF = new float [NUMBER_OF_NEURONS * NUMBER_OF_INPUTS];// Forget Gate
		WGO = new float [NUMBER_OF_NEURONS * NUMBER_OF_INPUTS];// Output Gate
		WCI = new float [NUMBER_OF_NEURONS * NUMBER_OF_INPUTS];// Memory Cell Input

		// Peepholes
		WIP = new float [NUMBER_OF_NEURONS];// Input Gate
		WFP = new float [NUMBER_OF_NEURONS];// Forget Gate
		WOP = new float [NUMBER_OF_NEURONS];// Output Gate

		// Output layer weigths
 		W2 = new float [NUMBER_OF_CLASSES * (1 + NUMBER_OF_NEURONS * 2)];
	}

	// Destructor
	NeuralNetwork::~NeuralNetwork()
	{
		delete[] WGI;
		delete[] WGF;
		delete[] WGO;
		delete[] WCI;

		delete[] WIP;
		delete[] WFP;
		delete[] WOP;

		delete[] W2;
	}

	void NeuralNetwork::Init(std::string inputFileModel)
	{
		std::ifstream inputStream;
		inputStream.open(inputFileModel, std::ifstream::in);

		if(!inputStream.good())
		{
			std::cerr << "ERROR: Failed to open " << inputFileModel << std::endl;
			return;
		}

		float weight;
		unsigned int local_count = 0, global_count = 0;

		// WGI NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
		while(inputStream >> weight)
		{
			WGI[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS * NUMBER_OF_INPUTS)
			{
				local_count = 0;
				break;
			}
		}

		// WGF NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
		while(inputStream >> weight)
		{
			WGF[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS * NUMBER_OF_INPUTS)
			{
				local_count = 0;
				break;
			}
		}

		// WGO NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
		while(inputStream >> weight)
		{
			WGO[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS * NUMBER_OF_INPUTS)
			{
				local_count = 0;
				break;
			}
		}

		// WCI NUMBER_OF_NEURONS
		while(inputStream >> weight)
		{
			WCI[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS * NUMBER_OF_INPUTS)
			{
				local_count = 0;
				break;
			}
		}

		// WIP NUMBER_OF_NEURONS
		while(inputStream >> weight)
		{
			WIP[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS)
			{
				local_count = 0;
				break;
			}

		}

		// WFP NUMBER_OF_NEURONS
		while(inputStream >> weight)
		{
			WFP[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS)
			{
				local_count = 0;
				break;
			}
		}

		// WOP NUMBER_OF_NEURONS
		while(inputStream >> weight)
		{
			WOP[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_NEURONS)
			{
				local_count = 0;
				break;
			}
		}

		// W2 NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1)
		while(inputStream >> weight)
		{
			W2[local_count] = weight;

			local_count++;
			global_count++;

			if(local_count == NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1))
			{
				local_count = 0;
				break;
			}
		}

		inputStream.close();

		if(global_count != NUMBER_OF_NEURONS * NUMBER_OF_INPUTS * 4 + NUMBER_OF_NEURONS * 3 + NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1))
		{
			std::cerr << "ERROR: Incorrect number of weights...!" << std::endl;
			return;
		}

	}



	//====================================================================================================================================================================================================================
	// ALPHABET
	//====================================================================================================================================================================================================================

	// Constructor
	Alphabet::Alphabet()
	{
	}

	// Destructor
	Alphabet::~Alphabet()
	{
		alphabet.clear();
	}

	void Alphabet::Init(std::string inputFileAlphabet)
	{
		std::ifstream inputStream;
		inputStream.open(inputFileAlphabet, std::ifstream::in);

		if(!inputStream.good())
		{
			std::cerr << "ERROR: Failed to open " << inputFileAlphabet << std::endl;
			return;
		}

		std::string symbol;

		while (getline(inputStream, symbol))
			alphabet.push_back(symbol);

		inputStream.close();

		if(alphabet.size() != NUMBER_OF_CLASSES)
			std::cerr << "ERROR: Wrong Number of Aplhabetic Symbols...!" << std::endl;

	}

	std::string Alphabet::ReturnSymbol(unsigned int label)
	{
		if(label < NUMBER_OF_CLASSES)
			return alphabet.at(label);
		else
		{
			std::cerr << "ERROR: The Class Number is out of Range...!" << std::endl;
			return alphabet.at(80);
		}
	}

	void Alphabet::Print()
	{
		for(unsigned int s = 0; s < NUMBER_OF_CLASSES; s++ )
		{
			std::cout << s << "   " << alphabet.at(s) << "   size: " << alphabet.at(s).size() << "   ";

			for(unsigned int i = 0; i < alphabet.at(s).size(); i++ )
				std::cout << (int)alphabet.at(s).at(i) << "   ";

			std::cout << std::endl;
		}

	}



	//====================================================================================================================================================================================================================
	// INPUT IMAGES
	//====================================================================================================================================================================================================================

	// Constructor
	InputImage::InputImage()
	{
		numberOfColumns = 0;
	}

	// Destructor
	InputImage::~InputImage()
	{
		if (image_fw != NULL)
		{
			delete[] image_fw;
			image_fw = NULL;
		}

		if (image_bw != NULL)
		{
			delete[] image_bw;
			image_bw = NULL;
		}
	}

	void InputImage::Init(std::string inputFileImage)
	{
		std::ifstream inputStream;
		inputStream.open(inputFileImage, std::ifstream::in);

		if(!inputStream.good())
		{
			std::cerr << "ERROR: Failed to open " << inputFileImage << std::endl;
			return;
		}

		// Temporal structure to store image
		std::vector<float> tmp;
		float pix;

		unsigned int values = 0;
		while((inputStream >> pix) && (values++ < MAX_NUMBER_COLUMNS_TEST_SET * HIGHT_IN_PIX))
		{
			if (values > BYPASS_COLUMNS * HIGHT_IN_PIX)
			tmp.push_back(pix);
		}

		inputStream.close();

		// Number of columns of the image has to be a multiple of HIGHT_IN_PIX
		if(tmp.size() % HIGHT_IN_PIX != 0)
		{
			std::cerr << "ERROR: Incorrect number of pixels...!" << std::endl;
			return;
		}

		numberOfColumns = tmp.size() / HIGHT_IN_PIX;

		image_fw = new float [numberOfColumns * HIGHT_IN_PIX];
		image_bw = new float [numberOfColumns * HIGHT_IN_PIX];

		for(unsigned int col = 0; col < numberOfColumns; col++)
		{
			for(unsigned int row = 0; row < HIGHT_IN_PIX; row++)
			{
				image_fw[col * HIGHT_IN_PIX + row] = tmp.at(col * HIGHT_IN_PIX + row);
				// Creat an image for backward processing: mirror the columns of the forward image
				image_bw[col * HIGHT_IN_PIX + row] = tmp.at((numberOfColumns - col - 1) * HIGHT_IN_PIX + row);
			}
		}
//exit(0);
		tmp.clear();
	}

	void InputImage::Free()
	{
		if (image_fw != NULL)
		{
			delete[] image_fw;
			image_fw = NULL;
		}

		if (image_bw != NULL)
		{
			delete[] image_bw;
			image_bw = NULL;
		}
	}

	void InputImage::Print()
	{
		unsigned int numberOfSamples = 10;

		std::cout << "image: ";

		for(unsigned int i = 0; i < numberOfSamples; i++)
			std::cout << image_fw[i] << " ";
		std::cout << " ... " << "image[C * N]: " << image_fw[numberOfColumns * HIGHT_IN_PIX - 2] << " " << image_fw[numberOfColumns * HIGHT_IN_PIX - 1] << std::endl;

	}




	//====================================================================================================================================================================================================================
	// GROUND TRUTH
	//====================================================================================================================================================================================================================

	// Constructor
	GroundTruth::GroundTruth()
	{
	}

	// Destructor
	GroundTruth::~GroundTruth()
	{
		groundTruth.clear();
	}

	void GroundTruth::Free()
	{
		groundTruth.clear();
	}

	void GroundTruth::Init(std::string inputFileGroundTruth)
	{
		std::ifstream inputStream;
		inputStream.open(inputFileGroundTruth, std::ifstream::in);

		if(!inputStream.good())
		{
			std::cerr << "ERROR: Failed to open " << inputFileGroundTruth << std::endl;
			return;
		}

		std::string symbol;

		while (getline(inputStream, symbol))
		{
			groundTruth = symbol;
		}

		inputStream.close();

	}

	std::string GroundTruth::ReturnString()
	{
		return groundTruth;
	}




	//====================================================================================================================================================================================================================
	// LSTM
	//====================================================================================================================================================================================================================

	// The dot product corresponding to a single gate of the LSTM memory cell
	inline float DotVectorToVector126(float *source, 	// IN  // size: 1.0 + HIGHT_IN_PIX + NUMBER_OF_NEURONS = NUMBER_OF_INPUTS
									  float *weights)	// IN  // size: NUMBER_OF_INPUTS
	{
		float output = 0.0;
		float tmp;

		for(unsigned int i = 0; i < NUMBER_OF_INPUTS; i++)
		{
			tmp = source[i] * weights[i];
			output += tmp;
		}

		return output;
	}

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
									 float *output)              	// OUT // A single output

	{
		float gix, gfx, gox, cix;
		float gi, gf, go, ci;
		float tmp_in_state, tmp_out_state;

		gix = DotVectorToVector126(source, WGI);
		gfx = DotVectorToVector126(source, WGF);
		gox = DotVectorToVector126(source, WGO);
		cix = DotVectorToVector126(source, WCI);

		tmp_in_state = in_state;

		if(currentColumn > 0)
		{
			gix = gix + WIP * tmp_in_state;
			gfx = gfx + WFP * tmp_in_state;
		}

		gi = 1.0/(1.0 + expf(-gix));
		gf = 1.0/(1.0 + expf(-gfx));


		ci = tanhf(cix);
		tmp_out_state = ci * gi;

		if(currentColumn > 0)
		{
			tmp_out_state = tmp_out_state + gf * tmp_in_state;
			gox = gox + WOP * tmp_out_state;
		}

		go =  1.0/(1.0 + expf(-gox));
		*output = tanhf(tmp_out_state) * go;

		*out_state = tmp_out_state;

	}

	void Hidden_Layer(float *image,					// IN  // size: numberOfColumns * HIGHT_IN_PIX
					  unsigned int numberOfColumns,	// IN  //
					  float *WGI, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WGF, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WGO, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WCI, 					// IN  // size: NUMBER_OF_NEURONS * NUMBER_OF_INPUTS
					  float *WIP, 					// IN  // size: NUMBER_OF_NEURONS
					  float *WFP, 					// IN  // size: NUMBER_OF_NEURONS
					  float *WOP,					// IN  // size: NUMBER_OF_NEURONS
					  float *result)				// OUT // size: numberOfColumns * NUMBER_OF_NEURONS

	{

		float source[NUMBER_OF_INPUTS];
		float outputRegister[NUMBER_OF_NEURONS];
		float stateRegister[NUMBER_OF_NEURONS];

		float out_state, output;

		float *pWGI, *pWGF, *pWGO, *pWCI;

		for(unsigned int i = 0; i < NUMBER_OF_NEURONS; i++)
			outputRegister[i] = 0.0;

		for(unsigned int column = 0; column < numberOfColumns; column++)
		{
			//Concatinate 1.0 + image + previous output
			source[0] = 1.0;
			std::memcpy(source + 1, image + column * HIGHT_IN_PIX, HIGHT_IN_PIX * 4);
			std::memcpy(source + 1 + HIGHT_IN_PIX, outputRegister, NUMBER_OF_NEURONS * 4);

			for(unsigned int n = 0; n < NUMBER_OF_NEURONS; n++)
			{

				pWGI = WGI + n * NUMBER_OF_INPUTS;
				pWGF = WGF + n * NUMBER_OF_INPUTS;
				pWGO = WGO + n * NUMBER_OF_INPUTS;
				pWCI = WCI + n * NUMBER_OF_INPUTS;

				HiddenLayerSingleMemoryCell(source,
										    column,
										    stateRegister[n],
										    pWGI,
										    pWGF,
										    pWGO,
										    pWCI,
										    WIP[n],
										    WFP[n],
										    WOP[n],
										    &out_state,
										    &output);


				stateRegister[n] = out_state;
				outputRegister[n] = output;
			}

			for(unsigned int i = 0; i < NUMBER_OF_NEURONS; i++)
				result[column * NUMBER_OF_NEURONS + i] = outputRegister[i];
		}
	}




	//====================================================================================================================================================================================================================
	// Connectionist Temporal Classification Layer (CTC layer)
	//====================================================================================================================================================================================================================

	// The dot product corresponding to a single neuron of the output layer operating on an concatinated output from the forward and the bakward hidden layers
	inline float DotVectorToVector201(float *W2,		// IN  // size: NUMBER_OF_NEURONS * 2 + 1
									  float *input_fw, 	// IN  // size: NUMBER_OF_NEURONS
							          float *input_bw)	// IN  // size: NUMBER_OF_NEURONS
	{
		float output = 0.0;
		float tmp;

		output = 1.0 * W2[0];

		for(unsigned int i = 0; i < NUMBER_OF_NEURONS; i++)
		{
			tmp = W2[1 + i] * input_fw[i];
			output += tmp;
		}

		for(unsigned int i = 0; i < NUMBER_OF_NEURONS; i++)
		{
			tmp = W2[1 + NUMBER_OF_NEURONS + i] * input_bw[i];
			output += tmp;
		}

		return output;
	}

	// The function of a single neuron of the output layer
	inline void OutputLayerSinlgleNeuron(float *W2, 		// IN  // size: NUMBER_OF_NEURONS * 2 + 1
										 float *input_fw,	// IN  // size: NUMBER_OF_NEURONS
										 float *input_bw, 	// IN  // size: NUMBER_OF_NEURONS
										 float *output)		// OUT //
	{
		*output = DotVectorToVector201(W2, input_fw, input_bw);
	}

	void Output_Layer(unsigned int numberOfColumns, // IN  //
					  float *W2, 					// IN  // size: NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1)
					  float *input_fw,			 	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  float *input_bw,			 	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  float *output) 				// OUT // size: numberOfColumns * NUMBER_OF_CLASSES
	{

		float sum;

		// Compute the function of the output layer for each concatinated column
		for(unsigned int col = 0; col < numberOfColumns; col++)
		{
			sum = 0.0;

			float *pInput_fw  = input_fw  + col * NUMBER_OF_NEURONS;
			float *pInput_bw  = input_bw  + (numberOfColumns - col - 1) * NUMBER_OF_NEURONS;
			float *pOutput = output + col * NUMBER_OF_CLASSES;

			// Compute the function of each neuron of the output layer
			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
			{
				float *pW2 = W2 + cl * (NUMBER_OF_NEURONS * 2 + 1);

				OutputLayerSinlgleNeuron(pW2, pInput_fw, pInput_bw, pOutput+cl);
			}

			// Softmax function
			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
			{
				*(pOutput+cl) = exp(*(pOutput+cl));
			}

			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
			{
				sum += *(pOutput+cl);
			}

			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
				*(pOutput+cl) /= sum;

		}
	}

	// Reconstruct a line from the labels
	void TranslateBack(Alphabet &alphabet, 				// IN  //
					   unsigned int numberOfColumns, 	// IN  //
					   float *input, 					// IN  // size: numberOfColumns * NUMBER_OF_CLASSES
					   std::string &output, 			// OUT //
					   float threshold)					// IN  //
	{
		unsigned int left_limit = 0, right_limit;
		unsigned int offset, label;

		for(unsigned int col = 0; col < numberOfColumns; col++)
		{
			if (input[col * NUMBER_OF_CLASSES] > threshold && input[(col + 1) * NUMBER_OF_CLASSES] < threshold )
				left_limit = (col + 1) * NUMBER_OF_CLASSES;
			else if (input[col * NUMBER_OF_CLASSES] < threshold && input[(col + 1) * NUMBER_OF_CLASSES] > threshold )
			{
				right_limit = (col + 1) * NUMBER_OF_CLASSES;
				offset = std::max_element(input + left_limit, input + right_limit) - input;
				label = offset - (offset / NUMBER_OF_CLASSES) * NUMBER_OF_CLASSES;

				std::string tmpSymbol = alphabet.ReturnSymbol(label);
				output.insert(output.end(), tmpSymbol.begin(), tmpSymbol.end() );

			}
		}

	}

	double LevenshteinDistance(const std::string& s1, const std::string& s2)
	{
		const std::size_t len1 = s1.size(), len2 = s2.size();
		std::vector<unsigned int> col(len2+1), prevCol(len2+1);

		for (unsigned int i = 0; i < prevCol.size(); i++)
			prevCol[i] = i;
		for (unsigned int i = 0; i < len1; i++)
		{
			col[0] = i+1;
			for (unsigned int j = 0; j < len2; j++)
			{
				//col[j+1] = std::min({ prevCol[1 + j] + 1, col[j] + 1, prevCol[j] + (s1[i]==s2[j] ? 0 : 1) });

				col[j+1] = std::min(prevCol[1 + j] + 1, std::min(col[j] + 1, prevCol[j] + (s1[i]==s2[j] ? 0 : 1)));
			}

			col.swap(prevCol);
		}
		#if PROFILE
			std::cout << prevCol[len2] << std::endl;
		#endif

		return (double)prevCol[len2] / (double)s2.size();
	}

	double LevenshteinDistanceCStyle(const char *s1, const char *s2)
	{
	    unsigned int s1len, s2len, x, y, lastdiag, olddiag;
	    s1len = strlen(s1);
	    s2len = strlen(s2);
	    unsigned int column[s1len+1];

	    for (y = 1; y <= s1len; y++)
			column[y] = y;

	    for (x = 1; x <= s2len; x++)
	    {
			column[0] = x;

			for (y = 1, lastdiag = x-1; y <= s1len; y++)
			{
				olddiag = column[y];
				column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
				lastdiag = olddiag;
			}
	    }

	    return (double)(column[s1len]) / (double)s1len;
	}

	//====================================================================================================================================================================================================================
	// AUXILIARY
	//====================================================================================================================================================================================================================

	bool directory_exists( const std::string &directory )
	{
	    if( !directory.empty() )
	    {
	        if( access(directory.c_str(), 0) == 0 )
	        {
	            struct stat status;
	            stat( directory.c_str(), &status );
	            if( status.st_mode & S_IFDIR )
	                return true;
	        }
	    }
	    // if any condition fails
	    return false;
	}


	std::vector<std::string> open(std::string path)
	{
		DIR*    dir;
		dirent* pdir;
		std::vector<std::string> files;

		std::cout << "Info: Opening path " << path << std::endl;

		if (!directory_exists(path)) {
			std::cout << "ERROR: Path " << path << " does not exist. Aborting" << std::endl;
			exit(-1);
		}

		dir = opendir(path.empty() ? "." : path.c_str());

		while ((pdir = readdir(dir))) {
			if ((strncmp(pdir->d_name, "..", 2) !=0) && (strncmp(pdir->d_name, ".", 1) !=0)) {
				//std::cout << "pushed " << pdir->d_name << std::endl;
				files.push_back(pdir->d_name);
			}
		}

		closedir(dir);

		if ((files.end() - files.begin()) <= 0) {
			std::cout << "No input files provided in path " << path << " Aborting..." << std::endl;
			exit(-1);
		}

		std::sort(files.begin(), files.end());
		//files.erase(files.begin(), files.begin()+2);

		std::cout << "INFO: Read " << (files.end() - files.begin()) << " files from path " <<  path << std::endl;

		if ((files.end() - files.begin()) > MAX_NUMBER_IMAGES_TEST_SET) {
			std::cout << "WARNING: Max number of files reached (MAX_NUMBER_IMAGES_TEST_SET = " \
					  << MAX_NUMBER_IMAGES_TEST_SET << " )" << std::endl;
			files.erase((files.begin() + MAX_NUMBER_IMAGES_TEST_SET), files.end());
		}

		return files;
	}

	unsigned int GetNumberOfThreads()
	{

		unsigned int numThreads;

		#pragma omp parallel
		{
			//int ID = omp_get_thread_num();
			//std::cout << ID << std::endl;

			numThreads = omp_get_num_threads();

		}
		//std::cout << "Number of threads on the current machine: " << numThreads << std::endl;

		return numThreads;
	}

	void print_usage_and_exit(const char *argv0)
	{
		std::cerr << "Usage: " << argv0 << " <parallelization> <path_data> <path_gt>" << std::endl;
		std::cerr << "    <parallelization>    : -s, -m" << std::endl;
		std::cerr << "    <path_data>          : ../../samples/" << std::endl;
		std::cerr << "    <path_gt>            : ../gt/" << std::endl;


		exit(1);
	}
