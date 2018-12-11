#!/usr/bin/env python3
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
#
# This script preprocesses the CIFAR-10 data set in multiple ways. For one, the
# input dataset is split into images and labels, and the images are converted to
# float32 format. For the training data set, the script can then apply various
# distortions to artificially augment the training data to help reduce
# overfitting. For the testing data set, the script centrally crops each image
# from 32x32 to 24x24 dimensions.

import numpy as np
import argparse
# from PIL import Image


# Dimensions of each image in the dataset.
IMAGE_HEIGHT = 32
IMAGE_WIDTH = 32
IMAGE_DEPTH = 3

# Calculate the data size of each record.
LABEL_BYTES = 1 # 1 for CIFAR-10, 2 for CIFAR-100
IMAGE_BYTES = IMAGE_HEIGHT * IMAGE_WIDTH * IMAGE_DEPTH
RECORD_BYTES = LABEL_BYTES + IMAGE_BYTES


def info(nda):
	return "shape=%s, size=%s, dtype=%s" % (nda.shape, nda.size, nda.dtype)


def distort_image(image):
	# Randomly crop the image to a 24x24 pixel subregion.
	cx, cy = np.random.random_integers(0, 8, [2])
	image = image[cx:(cx+24), cy:(cy+24)]

	# Toss a coin to see if the image should be flipped horizontally.
	flip = np.random.random_integers(0, 1)
	if flip == 1:
		image = np.fliplr(image) # along the width axis

	# Randomly change the brightness of the image.
	delta = np.random.random_integers(-63, 63)
	image += delta

	# Randomly adjust the contrast of the image.
	contrast = np.random.random() * (1.8-0.2) + 0.2
	mean = np.mean(image)
	image = (image - mean) * contrast + mean

	return image


def process_images(pixels):
	# Apply distortions if so requested, or just centrally crop the images to
	# 24x24 pixels.
	if args.distort:
		output = np.array([distort_image(i) for i in pixels])
	else:
		output = pixels[:,4:28,4:28,:]

	# print("output:", info(output))


	# Dump a sample image.
	# outpixel = np.asarray(np.clip(output, 0, 255), dtype=np.uint8)
	# pimg = Image.fromarray(outpixel[0:16].reshape(-1, 24, 3), "RGB")
	# pimg.save("outputs.png", "PNG")


	# Perform per-image standardization. This transforms each pixel x into y as
	# per:
	#
	# y = (x - mean) / adjusted_stddev
	# adjusted_stddev = max(stddev, 1.0 / sqrt(num_pixels))
	#
	# Where mean and stddev are the mean and standard deviation of the pixels in
	# each image.
	mean = np.mean(output, axis=(1,2,3), keepdims=True)
	# print("mean:", info(mean))
	stddev = np.std(output, axis=(1,2,3), keepdims=True)
	adjusted_stddev = np.maximum(stddev, 1.0/np.sqrt(np.prod(output.shape[1:])))
	# print("stddev:", info(stddev))
	# print("adjusted_stddev:", info(adjusted_stddev))
	output = (output - mean) / adjusted_stddev


	# Dump a sample image.
	# outpixel = np.asarray(np.clip(output * 64 + 128, 0, 255), dtype=np.uint8)
	# pimg = Image.fromarray(outpixel[0:16].reshape(-1, 24, 3), "RGB")
	# pimg.save("standardized.png", "PNG")

	return output


# Parse the input arguments.
parser = argparse.ArgumentParser(description="Preprocess the CIFAR-10 dataset.")
parser.add_argument("files", metavar="FILE", nargs="+", help="input file to process")
parser.add_argument("-o", "--output", metavar="OUTFILE", default="out.bin", help="name stem of the output files to produce")
parser.add_argument("-m", "--multiply", metavar="M", default="1", help="multiply the dataset", type=int)
parser.add_argument("--distort", action="store_true", help="distort the image in various ways")
args = parser.parse_args()
# print(args)


# Load all the input files into a contiguous chunk of memory.
print("loading %d files" % len(args.files))
raw_data = np.concatenate([np.fromfile(f, dtype=np.uint8) for f in args.files])
# print("loaded files")
# print("raw_data:", info(raw_data))

# Reshape the data into individual records.
records = raw_data.reshape(-1, RECORD_BYTES)
# print("records:", info(records))

# Slice the data into labels and pixel data. Then reshape the pixel data into
# the depth-height-width representation given in the input data, and transpose
# that into the height-width-depth required by our implementation.
labels = records[:,:LABEL_BYTES].reshape(-1)
pixels = records[:,LABEL_BYTES:].reshape(-1, IMAGE_DEPTH, IMAGE_HEIGHT, IMAGE_WIDTH).transpose(0, 2, 3, 1)
# print("labels:", info(labels))
# print("pixels:", info(pixels))


# Dump a sample image.
# pimg = Image.fromarray(pixels[0:16].reshape(-1, IMAGE_WIDTH, IMAGE_DEPTH), "RGB")
# pimg.save("inputs.png", "PNG")


# Convert the input image to float32.
pixels = np.asarray(pixels, dtype=np.float32)
# print("pixels:", info(pixels))


# Open the output files for writing.
with open(args.output+".data.bin", "w") as fd:
	with open(args.output+".labels.bin", "w") as fl:
		for i in range(args.multiply):
			print("processing images ... %d%% (%d/%d)" % (100 * (i+1) / args.multiply, i+1, args.multiply))
			output = process_images(pixels)
			labels.tofile(fl)
			output.tofile(fd)
		print("generated %d processed images" % (pixels.shape[0] * args.multiply))
