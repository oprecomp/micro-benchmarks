#!/usr/bin/env python3
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
#
# Runs the entire CIFAR-10 dataset through a previously trained model and
# evaluates per-image loss and correctness, as well as overall loss and accuracy
# across the entire dataset.

import tensorflow as tf
import numpy as np
import sys
import argparse
import subprocess
import shlex
import time

sys.path.append(sys.path[0]+"/../gd")
import cifar10


DATA_DIR = "data/prepared/mb/gd"
MODEL_PATH = "data/source/mb/cnn/model.ckpt"

parser = argparse.ArgumentParser(description="Performs classification of the entire CIFAR-10 dataset.")
parser.add_argument("-b", "--batch-size", default=100, type=int, help="batch size")
parser.add_argument("-m", "--amester", metavar="BIN", help="path to amester-tool (no measurement if omitted)")
args = parser.parse_args()

BATCH_SIZE = args.batch_size

# Print the batch size to stdout such that it ends up in the final results.
print("# batch_size: %d" % BATCH_SIZE)

def info(nda):
	return "shape=%s, size=%s, dtype=%s" % (nda.shape, nda.size, nda.dtype)

# Construct the network.
sys.stderr.write("constructing inference graph\n")
x = tf.placeholder(tf.float32, [None, 24, 24, 3])
y = cifar10.inference_graph(x)
y_ = tf.placeholder(tf.int32, [None])
all_losses = cifar10.losses(y, y_)
all_accuracies = cifar10.accuracies(y, y_)
saver = tf.train.Saver()


# Load the test data set.
test_images  = cifar10.open_images(DATA_DIR+"/cifar-10-test.data.bin")
test_labels  = cifar10.open_labels(DATA_DIR+"/cifar-10-test.labels.bin")
NUM_TEST_SAMPLES  = test_images.shape[0]
sys.stderr.write("loaded %d test data\n" % NUM_TEST_SAMPLES)


# Load the pre-trained model.
sess = tf.Session()
saver.restore(sess, MODEL_PATH)

# Evaluate the dataset.
sys.stderr.write("evaluating dataset\n")

if args.amester is not None:
	amester = subprocess.Popen(shlex.split(args.amester), stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)
	amester.stdin.write("{\n") # start measurement
	amester.stdin.flush()
	time.sleep(0.1)

while True:
	losses = list()
	accuracies = list()
	for i in range(NUM_TEST_SAMPLES // BATCH_SIZE):
		last_losses, last_accuracies = sess.run([all_losses, all_accuracies], feed_dict={
			x:  test_images[i*BATCH_SIZE:(i+1)*BATCH_SIZE],
			y_: test_labels[i*BATCH_SIZE:(i+1)*BATCH_SIZE],
		})
		losses.extend(last_losses)
		accuracies.extend(last_accuracies)

	# Check with AMESTER whether we should continue iterating.
	if args.amester is not None:
		amester.stdin.write(".\n") # ask whether we should continue
		amester.stdin.flush()
		if int(amester.stdout.readline()) != 1:
			break
		sys.stderr.write("extension requested by amester-tool\n")
	else:
		break

if args.amester is not None:
	amester.stdin.write("}\n") # stop measurement
	amester.stdin.flush()
	sys.stdout.write(amester.communicate()[0]) # wait for amester-tool to close, dump its stdout

# Prepare the gathered data and store to disk.
sys.stderr.write("gathering statistics\n")
losses = np.hstack(losses)
accuracies = np.hstack(accuracies)

with open("infer.dat", "w") as f:
	f.write("# sample\tlabel\tloss\tcorrect\n")
	for d in zip(np.arange(NUM_TEST_SAMPLES), test_labels, losses, accuracies):
		f.write("%d\t%d\t%g\t%d\n" % d)

with open("infer.stats", "w") as f:
	f.write("loss\t%g\n" % np.mean(losses))
	f.write("accuracy\t%g\n" % np.mean(accuracies))

sys.stderr.write("refer to infer.dat and infer.stats\n")

# Print the loss and accuracy to stdout such that it ends up in the results.
print("# loss: %g" % np.mean(losses))
print("# accuracy: %g" % np.mean(accuracies))

# Shut down the session.
sess.close()
