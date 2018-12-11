// Copyright (c) 2017 OPRECOMP Project
// Dionysios Diamantopoulos <did@zurich.ibm.com>

#ifndef NEURON_COM_H
#define NEURON_COM_H

// BLSTM options
#define NUMBER_OF_INPUTS 126
#define NUMBER_OF_NEURONS 100
#define HIGHT_IN_PIX 25
#define NUMBER_OF_CLASSES 110


// BLSTM execution specific options
#define MAX_NUMBER_COLUMNS_TEST_SET 732 // 732
#define BYPASS_COLUMNS 0
#define MAX_NUMBER_IMAGES_TEST_SET 1//3402 dataset size
#define MAX_PIXELS_PER_IMAGE 2 * (MAX_NUMBER_COLUMNS_TEST_SET - BYPASS_COLUMNS) * HIGHT_IN_PIX

/* The number of columns fed per single kernel execution.
 * In order to keep memory resources low, we can split the input columns and feed
 * them to the accelerator in chunks of x-columns. However, this affects the
 * accuracy, since the cut may happen at critical information. The more cutting
 * points, i.e. lower COLS_PER_KERNEL_EXEC, the higher probability of destroying
 * contemporal information, leading to less accuracy. When equals to
 * MAX_NUMBER_COLUMNS_TEST_SET, no splitting is forced.
 * */
#define COLS_PER_KERNEL_EXEC 50


#define RECONSTURCT_BW_IMAGE_IN_PULP

// Options for approximating activation functions
// 0 : No approximation, use of math.h functions
// 1 : Use of low-latency math approximation functions
// 2 : Approximation with look-up tables
#define TRIGF_APPROX 1

// Auxiliary
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define FLT_MAX 3.402823466e+38F /* max value */
#define FLT_MIN 1.175494351e-38F /* min positive value */


#endif // NEURON_COM_H
