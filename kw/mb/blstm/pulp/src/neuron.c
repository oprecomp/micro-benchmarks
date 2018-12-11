// Copyright (c) 2017 OPRECOMP Project
//
// Original BLSTM code Copyright (C) 2017 University of Kaiserslautern
// Microelectronic Systems Design Research Group
// Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
// 20. February 2017
//
// Ported to PULP by Dionysios Diamantopoulos <did@zurich.ibm.com>
// December 2017

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "rt/rt_api.h"

#include "../include/neuron.h"
#include "../include/model.h"
#include "../include/generate_luts.h"
#include "../include/lut.h"
#include "../include/tiny_math.h"



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



	// The dot product corresponding to a four gates of the LSTM memory cell
	void DotVectorToVector126_four(float source[NUMBER_OF_INPUTS],
									  float weights0[NUMBER_OF_INPUTS],
									  float weights1[NUMBER_OF_INPUTS],
									  float weights2[NUMBER_OF_INPUTS],
									  float weights3[NUMBER_OF_INPUTS],
									  float outputs[4])	// IN  // size: NUMBER_OF_INPUTS
	{
		outputs[0] = outputs[1] = outputs[2] = outputs[3] = 0.0;
		float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
		for(unsigned int i = 0; i < NUMBER_OF_INPUTS; i++)
		{

		float src_fw = source[i];
		tmp0 = src_fw * weights0[i];
		tmp1 = src_fw * weights1[i];
		tmp2 = src_fw * weights2[i];
		tmp3 = src_fw * weights3[i];
		outputs[0] += tmp0;
		outputs[1] += tmp1;
		outputs[2] += tmp2;
		outputs[3] += tmp3;
/*
		u.f = source[i];
		printf("DEBUG pulp: source[%lu]=%08lx\n", i, (uint32_t)u.t);

		u.f = weights0[i];
		printf("DEBUG pulp: weights0[%lu]=%08lx\n", i, (uint32_t)u.t);

		u.f = tmp0;
		printf("DEBUG pulp: tmp0=%08lx\n", (uint32_t)u.t);
*/
		}

/*
	multiplication issue:
	test numbers (1st is ok, 2nd-3rd produce wrong last digit):
	DEBUG pulp: source[0]=3f800000
	DEBUG pulp: weights0[0]=3d32f520
	DEBUG pulp: tmp0=3d32f520
	DEBUG pulp: source[1]=4027da0e
	DEBUG pulp: weights0[1]=3da81fcc
	DEBUG pulp: tmp0=3e5c77e4
	DEBUG pulp: source[3]=3e63a5bb
	DEBUG pulp: weights0[3]=bd5b31b5
	DEBUG pulp: tmp0=bc42eafa

	DEBUG x86: source[0]=3f800000
	DEBUG x86: weights0[0]=3d32f520
	DEBUG x86: tmp0=3d32f520
	DEBUG x86: source[1]=4027da0e
	DEBUG x86: weights0[1]=3da81fcc
	DEBUG x86: tmp0=3e5c77e5
	DEBUG x86: source[3]=3e63a5bb
	DEBUG x86: weights0[3]=bd5b31b5
	DEBUG x86: tmp0=bc42eafb
*/

	}



	float divexpf_lookup(float x) {
		const float step = ((float)fabs(MAX_TARGET_DIVEXPF)+(float)fabs(MIN_TARGET_DIVEXPF)) / (LUT_SIZE_DIVEXPF-1);
		if (x <= MIN_TARGET_DIVEXPF)
			return 0;
		else if (x >= MAX_TARGET_DIVEXPF)
			return 1;
		else {
			//Since the divisor is a constant, then the problem devolves into a multiplication by a constant (the reciprocal of the divisor).
			int index = (int)(((x-(MIN_TARGET_DIVEXPF))/step));
			//printf("DEBUG: divexpf_lut[%d]=%f, step=%f\n", index, divexpf_lut[index], step);
			return divexpf_lut[index];
		}
	}

		float tanh_lookup(float x) {
			const float step = ((float)fabs(MAX_TARGET_TANH)+(float)fabs(MIN_TARGET_TANH)) / (LUT_SIZE_TANH-1);
			if (x <= MIN_TARGET_TANH)
				return -1;
			else if (x >= MAX_TARGET_TANH)
				return 1;
			else {
				//Since the divisor is a constant, then the problem devolves into a multiplication by a constant (the reciprocal of the divisor).
				int index = (int)(((x-(MIN_TARGET_TANH))/step));
				//printf("DEBUG: tanh_lut[%d]=%f, step=%f\n", index, tanh_lut[index], step);
				return tanh_lut[index];
			}
		}


		float expf_lookup(float x) {
			const float step = ((float)fabs(MAX_TARGET_EXPF)+(float)fabs(MIN_TARGET_EXPF)) / (LUT_SIZE_EXPF-1);
			if (x <= MIN_TARGET_EXPF)
				return 0;
			else if (x >= MAX_TARGET_EXPF)
				return expf_lut[LUT_SIZE_EXPF-1];
			else {
				//Since the divisor is a constant, then the problem devolves into a multiplication by a constant (the reciprocal of the divisor).
				int index = (int)(((x-(MIN_TARGET_EXPF))/step));
				//printf("DEBUG: expf_lut[%d]=%f, step=%f\n", index, expf_lut[index], step);
				return expf_lut[index];
			}
		}


	// The function of a single LSTM memory cell
	void HiddenLayerSingleMemoryCell(float *source,	// IN  // size: 1.0 + HIGHT_IN_PIX + NUMBER_OF_NEURONS = NUMBER_OF_INPUTS
									 unsigned int currentColumn,		// IN  // The current column of the image
									 float in_state,				// IN  // A single input state
									 float *WGI,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WGF,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WGO,   					// IN  // size: NUMBER_OF_INPUTS
									 float *WCI,   					// IN  // size: NUMBER_OF_INPUTS
									 float WIP,							// IN  // A single peephole weight
									 float WFP,							// IN  // A single peephole weight
									 float WOP,							// IN  // A single peephole weight
									 float *out_state,			// OUT // A single output state
									 float *output)         // OUT // A single output

	{
		float gix, gfx, gox, cix, pulp_div_tmp;
		float gi, gf, go, ci;
		float tmp_in_state, tmp_out_state;
		float outputs[4];
		#if TRIGF_APPROX != 0
		float divider;
		#endif

		DotVectorToVector126_four(source, WGI, WGF, WGO, WCI, outputs);

		tmp_in_state = in_state;

		gix = outputs[0];
		gfx = outputs[1];
		gox = outputs[2];
		cix = outputs[3];

		//gix = DotVectorToVector126(source, WGI);
		//gfx = DotVectorToVector126(source, WGF);
		//gox = DotVectorToVector126(source, WGO);
		//cix = DotVectorToVector126(source, WCI);


		if(currentColumn > 0)
		{
			gix = gix + WIP * tmp_in_state;
			gfx = gfx + WFP * tmp_in_state;
		}

		#if TRIGF_APPROX == 0
		gi = 1.0/(1.0 + expf(-(float)gix));
		gf = 1.0/(1.0 + expf(-(float)gfx));
		ci = (float)tanhf((float)cix);
		#elif TRIGF_APPROX == 1
		divider = 1.0 + tiny_expf(-gix);
		gi = 1.0/divider;
		divider = 1.0 + tiny_expf(-gfx);
		gf = 1.0/divider;
		ci = tiny_tanhf(cix);
		#elif TRIGF_APPROX == 2
		gi = divexpf_lookup(gix);
		gf = divexpf_lookup(gfx);
		ci = tanh_lookup(cix);
		#endif

		tmp_out_state = ci * gi;

		if(currentColumn > 0)
			{
				tmp_out_state = tmp_out_state + gf * tmp_in_state;
				gox = gox + WOP * tmp_out_state;
			}
		#if TRIGF_APPROX == 0
			go = (float)(1.0/(1.0 + expf(-(float)gox)));
			*output = (float)tanhf((float)tmp_out_state) * go;
		#elif TRIGF_APPROX == 1
			divider = 1.0 + tiny_expf(-gox);
			go = 1.0/divider;
			*output = tiny_tanhf(tmp_out_state) * go;
		#elif TRIGF_APPROX == 2
			go = divexpf_lookup(gox);
			*output = tanh_lookup(tmp_out_state) * go;
		#endif

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

			//memcpy(source + 1, image + column * HIGHT_IN_PIX, HIGHT_IN_PIX * 4);
			//memcpy(source + 1 + HIGHT_IN_PIX, outputRegister, NUMBER_OF_NEURONS * 4);

			/*			union {
								uint32_t t;
								float f;
								char b[sizeof(float)];
						} u;
			*/
			for(unsigned int k = 0; k < HIGHT_IN_PIX; k++) {
				source[k+1] = image[column*HIGHT_IN_PIX+k];
				//u.f = source[k+1];
				//printf("image[%lu]=%08lx\n", column*HIGHT_IN_PIX+k, (unsigned int)u.t);
			}
			for(unsigned int k = 0; k < NUMBER_OF_NEURONS; k++) {
				source[k+1+HIGHT_IN_PIX] = outputRegister[k];
			}



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
	inline float DotVectorToVector201(float *W2,				// IN  // size: NUMBER_OF_NEURONS * 2 + 1
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
	inline void OutputLayerSinlgleNeuron(	float *W2, 				// IN  // size: NUMBER_OF_NEURONS * 2 + 1
										 									 	float *input_fw,	// IN  // size: NUMBER_OF_NEURONS
										 								 		float *input_bw, 	// IN  // size: NUMBER_OF_NEURONS
										 										float *output)		// OUT //
	{
		*output = DotVectorToVector201(W2, input_fw, input_bw);
	}

	void Output_Layer(unsigned int numberOfColumns, // IN  //
					  				float *W2, 				// IN  // size: NUMBER_OF_CLASSES * (NUMBER_OF_NEURONS * 2 + 1)
					  				float *input_fw,	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  				float *input_bw,	// IN  // size: numberOfColumns * NUMBER_OF_NEURONS
					  				float *output)		// OUT // size: numberOfColumns * NUMBER_OF_CLASSES
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

			/*			union {
								uint32_t t;
								float f;
								char b[sizeof(float)];
						} u;
			*/

			// Softmax function
			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
			{
				#if TRIGF_APPROX == 0
					pOutput[cl] = (float)expf((float)pOutput[cl]);
				#elif TRIGF_APPROX == 1
					pOutput[cl] = tiny_expf(pOutput[cl]);
					//u.f = pOutput[cl];
					//printf("EXP(pOutput[%lu])        =%08lx\n", cl, (unsigned int)u.t);
				#elif TRIGF_APPROX == 2
					pOutput[cl] = expf_lookup(pOutput[cl]);
					//u.f = pOutput[cl];
					//printf("expf_lookup(pOutput[%lu])=%08lx\n", cl, (unsigned int)u.t);
				#endif
			}

			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
			{
				sum += *(pOutput+cl);
			}

			for(unsigned int cl = 0; cl < NUMBER_OF_CLASSES; cl++)
				*(pOutput+cl) /= sum;

		}
	}

	/**
	 *  @brief  Return the index of a maximum element in a 1D array.
	 *  @param  first  Start index of range.
	 *  @param  last   End index of range.
	 *  @return  Index referencing the first instance of the largest value.
	*/
		unsigned int max_element_syn_test(float *image, unsigned int __first, unsigned int __last)
		{
		if (__first == __last)
			return __first;

			unsigned int __result = __first;
			while (++__first != __last) {
			//i++;
				float check1 = image[__result];
				float check2 = image[__first];
				if (check1 < check2)
				__result = __first;
			}
			//std::cout << "i = " << i << std::endl;
			return __result;
		}

	// Reconstruct a line from the labels
	void TranslateBack( 				// IN  //
					   unsigned int numberOfColumns, 	// IN  //
					   float *input, 					// IN  // size: numberOfColumns * NUMBER_OF_CLASSES
					   unsigned int *output, 			// OUT //
						 unsigned int *str_len,
					   float threshold)					// IN  //
	{
		unsigned int left_limit = 0, right_limit;
		unsigned int offset, label;
		*str_len = 0;

		for(unsigned int col = 0; col < numberOfColumns-1; col++)
		{
			if (input[col * NUMBER_OF_CLASSES] > threshold && input[(col + 1) * NUMBER_OF_CLASSES] < threshold )
				left_limit = (col + 1) * NUMBER_OF_CLASSES;
			else if (input[col * NUMBER_OF_CLASSES] < threshold && input[(col + 1) * NUMBER_OF_CLASSES] > threshold )
			{
				right_limit = (col + 1) * NUMBER_OF_CLASSES;
				offset = max_element_syn_test(input, left_limit, right_limit);
				label = offset - (offset / NUMBER_OF_CLASSES) * NUMBER_OF_CLASSES;
				output[*str_len] = label;
				*str_len = *str_len + 1;
			}
		}

	}



	void Single_Kernel_BLSTM(
			float *image_fw,
			float *image_bw,
			unsigned int numberOfColumns,
			unsigned int *vecPredictedStringInd,
			unsigned int *str_len)
	{

		//#ifdef DMM_ALLOC
		float *pOutputFromtHiddenLayer_fw = (float *)rt_alloc(RT_ALLOC_CL_DATA, numberOfColumns * NUMBER_OF_NEURONS * sizeof (float));
		if(!pOutputFromtHiddenLayer_fw) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }
		//float *poutputFromOutputLayer = (float *)rt_alloc(RT_ALLOC_CL_DATA, numberOfColumns * NUMBER_OF_CLASSES * sizeof (float));
		//if(!poutputFromOutputLayer) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }
		//#else
		//float pOutputFromtHiddenLayer_fw[numberOfColumns * NUMBER_OF_NEURONS];
		float pOutputFromtHiddenLayer_bw[COLS_PER_KERNEL_EXEC * NUMBER_OF_NEURONS];
		float poutputFromOutputLayer[COLS_PER_KERNEL_EXEC * NUMBER_OF_CLASSES];
		//#endif // DMM_ALLOC

		// Forward direction
		Hidden_Layer(image_fw,
				 numberOfColumns,
				 WGI_fw,
				 WGF_fw,
				 WGO_fw,
				 WCI_fw,
				 WIP_fw,
				 WFP_fw,
				 WOP_fw,
				 pOutputFromtHiddenLayer_fw);

		rt_free(RT_ALLOC_CL_DATA, image_fw, numberOfColumns * HIGHT_IN_PIX * sizeof (float));

		//float *pOutputFromtHiddenLayer_bw = (float *)rt_alloc(RT_ALLOC_CL_DATA, numberOfColumns * NUMBER_OF_NEURONS * sizeof (float));
		//if(!pOutputFromtHiddenLayer_bw) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }

		// Backward direction
		Hidden_Layer(image_bw,
				 numberOfColumns,
				 WGI_bw,
				 WGF_bw,
				 WGO_bw,
				 WCI_bw,
				 WIP_bw,
				 WFP_bw,
				 WOP_bw,
				 pOutputFromtHiddenLayer_bw);

		rt_free(RT_ALLOC_CL_DATA, image_bw, numberOfColumns * HIGHT_IN_PIX * sizeof (float));

		//float *poutputFromOutputLayer = (float *)rt_alloc(RT_ALLOC_CL_DATA, numberOfColumns * NUMBER_OF_CLASSES * sizeof (float));
		//if(!poutputFromOutputLayer) { printf("ERROR: PULP out of memory. Aborting...\n"); exit(-1); }

		// CTC - Output Layer
		Output_Layer(numberOfColumns,
				 W2,
				 pOutputFromtHiddenLayer_fw,
				 pOutputFromtHiddenLayer_bw,
				 poutputFromOutputLayer);

		// Return the predicted string
		TranslateBack(numberOfColumns, poutputFromOutputLayer, vecPredictedStringInd, str_len, 0.7);

   	//rt_free(RT_ALLOC_CL_DATA, poutputFromOutputLayer, numberOfColumns * NUMBER_OF_CLASSES * sizeof (float));
	  rt_free(RT_ALLOC_CL_DATA, pOutputFromtHiddenLayer_fw, numberOfColumns * NUMBER_OF_NEURONS * sizeof (float));
	  //rt_free(RT_ALLOC_CL_DATA, pOutputFromtHiddenLayer_bw, numberOfColumns * NUMBER_OF_NEURONS * sizeof (float));
}




	void print_usage_and_exit(const char *argv0)
	{
		printf("Usage: %s  <num_columns>", argv0);

		exit(1);
	}
