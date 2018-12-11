#!/usr/bin/env python3
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
#
# The code in this file is roughly based on [1].
#
# [1]: https://www.tensorflow.org/tutorials/deep_cnn

import tensorflow as tf
import cifar10
import sys
import time
import datetime
import os
import numpy as np
import argparse
import subprocess
import shlex
import time
# from PIL import Image


DATA_DIR = "data/prepared/mb/gd"

parser = argparse.ArgumentParser(description="Trains a network on the CIFAR-10 dataset.")
parser.add_argument("-b", "--batch-size", default=100, type=int, help="batch size")
parser.add_argument("-n", "--num-steps", default=100000, type=int, help="number of steps")
parser.add_argument("--no-eval", action="store_true", help="do not periodically evaluate accuracy")
parser.add_argument("--no-save", action="store_true", help="do not periodically save the model")
parser.add_argument("-m", "--amester", metavar="BIN", help="path to amester-tool (no measurement if omitted)")
args = parser.parse_args()

NUM_STEPS  = args.num_steps  # number of total steps (batches) to learn
BATCH_SIZE = args.batch_size # number of examples per batch
LOG_FREQ   = 100       # log the training performance every N steps
SAVE_FREQ  = 10000     # save the trained model every N steps
EVAL_FREQ  = 1000      # evaluate the trained model every N steps

if args.no_save:
	SAVE_FREQ = None
if args.no_eval:
	EVAL_FREQ = None

# Print the number of steps and batch size to stdout such that it ends up in the
# final results.
print("# num_steps: %d" % NUM_STEPS)
print("# batch_size: %d" % BATCH_SIZE)

def info(nda):
	return "shape=%s, size=%s, dtype=%s" % (nda.shape, nda.size, nda.dtype)

# Construct the network.
sys.stderr.write("constructing training graph\n")
x = tf.placeholder(tf.float32, [None, 24, 24, 3], name="x")
y = cifar10.inference_graph(x)
y_ = tf.placeholder(tf.int32, [None], name="y_")
global_step = tf.placeholder(tf.int32, name="global_step")
all_losses = cifar10.losses(y, y_)
loss = tf.reduce_mean(all_losses)
all_accuracies = cifar10.accuracies(y, y_)
accuracy = tf.reduce_mean(all_accuracies)
lr = tf.train.exponential_decay(
	0.1,         # initial learning rate
	global_step, # step variable
	20000,       # steps until decay
	0.1,         # decay factor
	staircase=True
)
train_step = tf.train.GradientDescentOptimizer(lr).minimize(loss)
saver = tf.train.Saver()


# Load the training and test data sets.
sys.stderr.write("loading data sets\n")
test_images  = cifar10.open_images(DATA_DIR+"/cifar-10-test.data.bin")
test_labels  = cifar10.open_labels(DATA_DIR+"/cifar-10-test.labels.bin")
train_images = cifar10.open_images(DATA_DIR+"/cifar-10-train.data.bin")
train_labels = cifar10.open_labels(DATA_DIR+"/cifar-10-train.labels.bin")
NUM_TRAIN_SAMPLES = train_images.shape[0]
NUM_TEST_SAMPLES  = test_images.shape[0]
sys.stderr.write("loaded %d train and %d test data\n" % (NUM_TRAIN_SAMPLES, NUM_TEST_SAMPLES))

# Calculate the number of batches per epoch to be performed.
TRAIN_EPOCH = NUM_TRAIN_SAMPLES // BATCH_SIZE
TEST_EPOCH  = NUM_TEST_SAMPLES  // BATCH_SIZE
sys.stderr.write("epoch size: %d (train), %d (test)\n" % (TRAIN_EPOCH, TEST_EPOCH))

# Determine a name for the output files and open them for writing.
i = 0
while True:
	i += 1
	suffix = "%s_%03d.dat" % (datetime.datetime.today().strftime("%Y_%m_%d"), i)
	step_file = "step_"+suffix
	eval_file = "eval_"+suffix
	if not os.path.exists(step_file) and not os.path.exists(eval_file):
		break
sys.stderr.write("output to: %s\n" % step_file)
sys.stderr.write("      and: %s\n" % eval_file)
step_file = open(step_file, "w")
eval_file = open(eval_file, "w")
start_time = time.time()

# Define a few logging functions that write to the above output files.
step_file.write("# step\ttime [s]\tloss min\tloss median\tloss max\tlearning rate\n")
def log_step(step, loss_min, loss_median, loss_max, rate):
	sys.stderr.write("step %d: loss=%g, lr=%g\n" % (step, loss_median, rate))
	step_file.write("%d\t%d\t%g\t%g\t%g\t%g\n" % (
		step,
		time.time() - start_time,
		loss_min,
		loss_median,
		loss_max,
		rate
	))
	step_file.flush()

eval_file.write("# step\ttime [s]\ttest loss\ttest accuracy\ttrain loss\ttrain accuracy\tlearning rate\n")
def log_eval(step, test_loss, test_acc, train_loss, train_acc, rate):
	sys.stderr.write("step %d: accuracy test=%.3g%% train=%.3g%%\n" % (step, test_acc*100, train_acc*100))
	eval_file.write("%d\t%d\t%g\t%g\t%g\t%g\t%g\n" % (
		step,
		time.time() - start_time,
		test_loss,
		test_acc,
		train_loss,
		train_acc,
		rate
	))
	eval_file.flush()

# Prepare the main training loop.
sess = tf.Session()
sess.run(tf.global_variables_initializer())

# Define a function that will evaluate the performance of the network on a data
# set.
def eval(name, images, labels, limit=None):
	if limit is not None:
		images = images[0:limit]
		labels = labels[0:limit]
	assert(images.shape[0] == labels.shape[0])

	num_steps = images.shape[0] // BATCH_SIZE
	results = list()

	for i in range(num_steps):
		sys.stderr.write("evaluating on %s dataset ... %d/%d\r" % (name, i, num_steps))
		sys.stderr.flush()
		results.append(sess.run([all_losses, all_accuracies], feed_dict={
			x:  images[i*BATCH_SIZE:(i+1)*BATCH_SIZE],
			y_: labels[i*BATCH_SIZE:(i+1)*BATCH_SIZE],
		}))

	return np.mean(np.hstack(results), 1)


# Run the main training loop.
shuffled_images = None
shuffled_labels = None
shuffle_offset = 0
losses = list()

if args.amester is not None:
	amester = subprocess.Popen(shlex.split(args.amester), stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)
	amester.stdin.write("{\n") # start measurement
	amester.stdin.flush()
	time.sleep(0.1)

while True:
	for step in range(NUM_STEPS):

		# Mark the beginning of each epoch.
		if step % TRAIN_EPOCH == 0:
			sys.stderr.write("epoch %d\n" % (step // TRAIN_EPOCH + 1))

		# Save the model every now and then.
		if not SAVE_FREQ is None and step % SAVE_FREQ == 0:
			path = saver.save(sess, "model.ckpt")
			sys.stderr.write("saved model as %s\n" % path)

		# Draw a new batch of samples from the dataset.
		permutation = np.random.randint(0, NUM_TRAIN_SAMPLES, [BATCH_SIZE])
		batch_images = train_images[permutation,...]
		batch_labels = train_labels[permutation,...]

		# Perform the training step.
		_, last_loss, last_rate = sess.run([train_step, loss, lr], feed_dict={
			x:  batch_images,
			y_: batch_labels,
			global_step: step,
		})
		losses.append(last_loss)

		# Log some information about the learning procedure every now and then.
		if step % LOG_FREQ == 0:
			losses = sorted(losses)
			loss_min = losses[0]
			loss_max = losses[-1]
			loss_median = losses[len(losses) // 2]
			losses = list()
			log_step(step, loss_min, loss_median, loss_max, last_rate)

		# Evaluate the model every now and then.
		if not EVAL_FREQ is None and step % EVAL_FREQ == 0:
			test_loss,  test_acc  = eval("test",  test_images,  test_labels)
			train_loss, train_acc = eval("train", train_images, train_labels, limit=50000)
			log_eval(step, test_loss, test_acc, train_loss, train_acc, last_rate)

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

# Save the final state of the model.
path = saver.save(sess, "model.ckpt")
sys.stderr.write("final model saved as %s\n" % path)

# Shut down the session.
sess.close()
