// Copyright (c) 2017 OPRECOMP Project
// Dionysios Diamantopoulos <did@zurich.ibm.com>

#include <stdio.h>
#include <stdlib.h>
#include <liboprecomp.h>

/* BLSTM specific header files */
#include <sstream>
#include <assert.h>
#include "neuron.hpp"

std::string inputFileImageDir, inputFileGroundTruthDir;

struct wed {
	uint64_t num_words;
	uint64_t *input;
	volatile uint64_t *output;
};


int main(int argc, char **argv) {
	opc_error_t err;

	// Open the PULP binary based on what has been passed on the command line.
	if (argc != 2) {
		fprintf(stderr, "usage: %s BINARY <no_arg>\n", argv[0]);
		return 1;
	}

	// Load the kernel.
	opc_kernel_t knl = opc_kernel_new();
	err = opc_kernel_load_file(knl, argv[1]);
	if (err != OPC_OK) {
		opc_perror("opc_kernel_load_file", err);
		return 1;
	}

	// The initialization of the alphabet
	Alphabet alphabet;
	alphabet.Init("../../../mb/blstm/alphabet/alphabet.txt");

	inputFileImageDir = "../../../mb/data/prepared/mb/blstm/fraktur_dataset/samples/";
	inputFileGroundTruthDir = "../../../mb/data/prepared/mb/blstm/gt/";

	//inputFileImageDir = "./sample_data/samples_1/";
	//inputFileGroundTruthDir = "./sample_data/gt_1/";

	// Return the list of images' file names
	std::vector<std::string> listOfImages = open(inputFileImageDir);
	unsigned int imgs = listOfImages.size();
	std::cout << "DEBUG: imgs = " << imgs << "\n";
	// Return the list of ground truth' file names
	std::vector<std::string> listOfGroundTruth = open(inputFileGroundTruthDir);
	unsigned int imgs_gd = listOfImages.size();
	std::cout << "DEBUG: imgs_gd = " << imgs_gd << "\n";
	/* Check that there are equal number of image files and groundtruth files */
	assert(((imgs == imgs_gd) != 0) && ("#Input Images / #Groundtruth shall be equal and at least one."));


	//----------------------------------------------------------------------
	// Read Images
	//----------------------------------------------------------------------

	std::vector<InputImage> vecInputImage;
	vecInputImage.resize(listOfImages.size());

	for(unsigned int i = 0; i < listOfImages.size(); i++) {
		std::string inputFileImage = inputFileImageDir + listOfImages.at(i);
		vecInputImage.at(i).Init(inputFileImage);
	}

	std::cout << "DEBUG: listOfImages.size() = " << listOfImages.size() << "\n";

	//----------------------------------------------------------------------
	// Read Ground Truth
	//----------------------------------------------------------------------

	std::vector<GroundTruth> vecGroundTruth;
	vecGroundTruth.resize(listOfImages.size());

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

	// Verify that we hold enough space for the loaded images of folder provided
	assert(listOfImages.size() <= MAX_NUMBER_IMAGES_TEST_SET);

	/* Allocate space for predicted length and string ids */
	unsigned int *vecPredictedStringLen = (unsigned int *)malloc(listOfImages.size()\
	 * sizeof(unsigned int ));
	assert (vecPredictedStringLen != NULL);
	unsigned int **vecPredictedStringInd = (unsigned int **)malloc(listOfImages.size()\
	 * sizeof(unsigned int *));
	assert (vecPredictedStringInd != NULL);
	for(unsigned int i = 0; i < listOfImages.size(); i++) {
		vecPredictedStringInd[i] = (unsigned int *)malloc((MAX_NUMBER_COLUMNS_TEST_SET -\
	 BYPASS_COLUMNS) * sizeof(unsigned int));
	 assert (vecPredictedStringInd[i] != NULL);
 	}

	double *error = new double [listOfImages.size()];
	double errorSum = 0.0;
	double accuracy = 0.0;

	short unsigned int numberOfColumnsVec[MAX_NUMBER_IMAGES_TEST_SET];

	//====================================================================================================================================================================================================================
	// START
	//====================================================================================================================================================================================================================

	std::cout << "Start ..." << std::endl;

	// Starting point - the first time stemp
	time_t t1 = time(0);

	uint64_t *ibuff = (uint64_t *)calloc(MAX_PIXELS_PER_IMAGE, sizeof(uint64_t));
	uint64_t *obuff = (uint64_t *)calloc((MAX_NUMBER_COLUMNS_TEST_SET - \
																			 BYPASS_COLUMNS), sizeof(uint64_t));

	union {
			uint32_t t;
			float f;
			char b[sizeof(float)];
	} u;

	// Prepare a buffer of input words, and a buffer for the outputs.
	struct wed wed = { .num_words = 0, .input = NULL, .output = NULL };
	wed.input  = ibuff;
	wed.output = obuff;

	for(unsigned int i = 0; i < vecInputImage.size(); i++) {
		/* Keep up to MAX_NUMBER_COLUMNS_TEST_SET columns */
		numberOfColumnsVec[i] = vecInputImage.at(i).numberOfColumns;

		/* copy actual fw/bw data of every image */
		for(unsigned int k = 0; k <  numberOfColumnsVec[i] * HIGHT_IN_PIX; k++) {
			u.f = (float)vecInputImage.at(i).image_fw[k];
			ibuff[k] =  u.t;
		 	/*std::cout << "DEBUG: fw index = " << k << ", vecInputImage.at(" << i << ").image_fw[" << k << "]=" << \
				 	vecInputImage.at(i).image_fw[k] << "\n";
			*/
		}
		#ifndef RECONSTURCT_BW_IMAGE_IN_PULP
		for(unsigned int k = 0 ; k < numberOfColumnsVec[i] * HIGHT_IN_PIX; k++) {
			u.f = (float)vecInputImage.at(i).image_bw[k];
			ibuff[numberOfColumnsVec[i] * HIGHT_IN_PIX + k] =  u.t;
			/*std::cout << "DEBUG: bw index = " << numberOfColumnsVec[i] * HIGHT_IN_PIX + k << ", vecInputImage.at(" << i << ").image_bw[" << k << "]=" << \
			vecInputImage.at(i).image_bw[k] << "\n";
			*/
		}
		#endif // RECONSTURCT_BW_IMAGE_IN_PULP

		/* Update the number of pixels */
		#ifdef RECONSTURCT_BW_IMAGE_IN_PULP
		unsigned int total_pixels = numberOfColumnsVec[i] * HIGHT_IN_PIX;
		#else
		unsigned int total_pixels = 2 * numberOfColumnsVec[i] * HIGHT_IN_PIX;
		#endif

		wed.num_words = total_pixels;

		std::cout << "INFO: numberOfColumnsVec[" << i << "] = " <<  numberOfColumnsVec[i] << std::endl;

		// erase output FIXME: it is normally not needed if we pass the ids found from pulp
		memset(obuff, 0, (MAX_NUMBER_COLUMNS_TEST_SET - \
										 BYPASS_COLUMNS) * sizeof(uint64_t));

		// Open a device and offload a job.
		opc_dev_t dev = opc_dev_new();
		err = opc_dev_open_any(dev);
		if (err != OPC_OK) {
			opc_perror("opc_dev_open_any", err);
			return 1;
		}

		err = opc_dev_launch(dev, knl, &wed, NULL);
		if (err != OPC_OK) {
			opc_perror("opc_dev_launch", err);
			return 1;
		}
		opc_dev_wait_all(dev);
		opc_dev_free(dev);

		std::string groundTruthstring = vecGroundTruth.at(i).ReturnString();
		vecPredictedStringLen[i] = wed.output[0];
		printf("INFO host: PULP returned %u ids for image %u\n", vecPredictedStringLen[i], i);
		for (uint64_t j = 0; j < vecPredictedStringLen[i]; j++) {
			if (wed.output[j] < NUMBER_OF_CLASSES) // check not out of max alphabet id
				vecPredictedStringInd[i][j] = wed.output[j+1]; // 1st address is length
			else {
				vecPredictedStringInd[i][j] = 0;
				printf("WARNING host: returned id=%lu is not valid, replaced by 0.\n", wed.output[j+1]);
			}
		}
	} // loop over all images

	// Ending point - the final time stemp
	time_t t2 = time(0);

// Do the translation from alphabet indexers to actual characters
// Since some special characters reserve 2-3 char positions, we do the translation
// to the SW, using string vectors, i.e. dynamic alloc, (avoiding 2D buffers on HW)
for(unsigned int i = 0; i < listOfImages.size(); i++) {
	for(unsigned int j = 0; j < vecPredictedStringLen[i]; j++) {
		std::string tmpSymbol = alphabet.ReturnSymbol(vecPredictedStringInd[i][j]);
		vecPredictedString.at(i).insert(vecPredictedString.at(i).end(), tmpSymbol.begin(), tmpSymbol.end() );
	}
}


	for(unsigned int i = 0; i < listOfImages.size(); i++) {
		//----------------------------------------------------------------------
		// Calculate Levenshtein Distance for each string and output result
		//----------------------------------------------------------------------
		std::string groundTruthstring = vecGroundTruth.at(i).ReturnString();
		error[i] = LevenshteinDistance(vecPredictedString.at(i), groundTruthstring);
		std::cout << i << " Expected: "<< groundTruthstring \
				  << "\n Predicted: " << vecPredictedString.at(i) << " Accuracy: " << (1-error[i])*100 << " %\n";
			std::cout << " Predicted id: ";
		for(unsigned int j = 0; j < vecPredictedStringLen[i]; j++)
			std::cout << vecPredictedStringInd[i][j] << " ";
		std::cout << std::endl;


			#if PROFILE
			std::cout << vecPredictedString.at(i) << std::endl;
			std::cout << groundTruthstring << std::endl << std::endl;
			#endif
			vecPredictedString.at(i).clear();
		groundTruthstring.clear();
	}

	for(unsigned int e = 0; e < listOfImages.size(); e++)
		errorSum += error[e];
	accuracy = (1.0 - errorSum / (float)listOfImages.size()) * 100.0;
	std::cout << "Total Accuracy for " <<  listOfImages.size() << " images: " \
	<<  accuracy << "%" << std::endl;

	errorSum = 0.0;

	/* deallocate mem */
	for(unsigned int i = 0; i < listOfImages.size(); i++)
		free (vecPredictedStringInd[i]);
	free(vecPredictedStringInd);
	free(vecPredictedStringLen);

  delete[] error;


	double time_span = difftime( t2, t1);
	std::cout << "Measured time ... " << time_span << " seconds" << std::endl << std::endl;





	// Clean up.
	//opc_dev_free(dev);
	opc_kernel_free(knl);
	return 0;
}
