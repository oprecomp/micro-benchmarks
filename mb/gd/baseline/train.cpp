// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
//
// The code in this file is a general adaptation of [1], governed by a BSD-style
// license that can be found in the LICENSE file in [1].
//
// [1]: https://github.com/tiny-dnn/tiny-dnn/blob/master/examples/mnist/train.cpp

#include <iostream>
#include "tiny_dnn/tiny_dnn.h"


static void
construct_net(
	tiny_dnn::network<tiny_dnn::sequential> &nn,
	tiny_dnn::core::backend_t backend_type
) {
	// Connection table according to [Y.Lecun, 1998 Table.1]
	#define O true
	#define X false
	static const bool tbl[] = {
		O, X, X, X, O, O, O, X, X, O, O, O, O, X, O, O,
		O, O, X, X, X, O, O, O, X, X, O, O, O, O, X, O,
		O, O, O, X, X, X, O, O, O, X, X, O, X, O, O, O,
		X, O, O, O, X, X, O, O, O, O, X, X, O, X, O, O,
		X, X, O, O, O, X, X, O, O, O, O, X, O, O, X, O,
		X, X, X, O, O, O, X, X, O, O, O, O, X, O, O, O
	};
	#undef O
	#undef X

	// construct nets
	//
	// C : convolution
	// S : sub-sampling
	// F : fully connected
	nn
	// layer 1: convolution: 1x32x32 -> 6x28x28
	<< tiny_dnn::convolutional_layer<tiny_dnn::activation::tan_h>(
		32, // input width
		32, // input height
		5,  // window size
		1,  // input channels
		6,  // output channels
		tiny_dnn::padding::valid, // no padding, output smaller than input
		true, // has bias
		1, // width stride
		1, // height stride
		backend_type)

	// layer 2: subsampling: 6x28x28 -> 6x14x14
	<< tiny_dnn::average_pooling_layer<tiny_dnn::activation::tan_h>(
		28, // input width
		28, // input height
		6,  // input channels
		2)  // pool size

	// layer 3: convolution: 6x14x14 -> 16x10x10
	<< tiny_dnn::convolutional_layer<tiny_dnn::activation::tan_h>(
		14, // input width
		14, // output width
		5,  // window size
		6,  // input channels
		16, // output channels
		tiny_dnn::core::connection_table(tbl, 6, 16), // use sparse weights
		tiny_dnn::padding::valid,
		true, // has bias
		1, // width stride
		1, // height stride
		backend_type)

	// layer 4: subsampling: 16x10x10 -> 16x5x5
	<< tiny_dnn::average_pooling_layer<tiny_dnn::activation::tan_h>(
		10, // input width
		10, // input height
		16, // input channels
		2)  // pool size

	// layer 5: convolution: 16x5x5 -> 120x1x1
	<< tiny_dnn::convolutional_layer<tiny_dnn::activation::tan_h>(
		5, // input width
		5, // input height
		5, // window size
		16, // input channels
		120, // output channels
		tiny_dnn::padding::valid,
		true, // has bias
		1, // width stride
		1, // height stride
		backend_type)

	// layer 6: fully connected: 120 -> 10
	<< tiny_dnn::fully_connected_layer<tiny_dnn::activation::tan_h>(
		120, // input size
		10,  // output size
		true, // has bias
		backend_type);
}


// static void
// train_network(
// 	const std::string &data_dir_path,
// 	double learning_rate,
// 	int n_epochs,
// 	int n_minibatch,
// 	tiny_dnn::core::backend_t backend_type
// ) {
// }

static tiny_dnn::core::backend_t
parse_backend_name(const std::string &name) {
	const std::array<std::pair<std::string, tiny_dnn::core::backend_t>, 5> names = {
		std::make_pair("internal", tiny_dnn::core::backend_t::internal),
		std::make_pair("nnpack"  , tiny_dnn::core::backend_t::nnpack  ),
		std::make_pair("libdnn"  , tiny_dnn::core::backend_t::libdnn  ),
		std::make_pair("avx"     , tiny_dnn::core::backend_t::avx     ),
		std::make_pair("opencl"  , tiny_dnn::core::backend_t::opencl  ),
	};
	for (const auto p : names) {
		if (p.first == name)
			return p.second;
	}
	if (name == "default")
		return tiny_dnn::core::default_engine();
	std::cerr << "unknown backend " << name << "\n";
	exit(1);
}


static void
train_lenet(
	const std::string &data_dir_path,
	double learning_rate,
	const int n_train_epochs,
	const int n_minibatch,
	tiny_dnn::core::backend_t backend_type
) {
	// Construct the network.
	tiny_dnn::network<tiny_dnn::sequential> nn;
	construct_net(nn, backend_type);

	// Load the MNIST database.
	std::vector<tiny_dnn::label_t> train_labels, test_labels;
	std::vector<tiny_dnn::vec_t> train_images, test_images;

	std::cout << "Loading training data...\n";
	tiny_dnn::parse_mnist_labels(
		data_dir_path + "/train-labels.idx1-ubyte",
		&train_labels
	);
	tiny_dnn::parse_mnist_images(
		data_dir_path + "/train-images.idx3-ubyte",
		&train_images,
		-1.0, // remap 0 to -1.0
		1.0,  // remap 255 to 1.0
		2,    // add two pixels of padding horizontally
		2     // add two pixels of padding vertically
	);

	std::cout << "Loading testing data...\n";
	tiny_dnn::parse_mnist_labels(
		data_dir_path + "/t10k-labels.idx1-ubyte",
		&test_labels
	);
	tiny_dnn::parse_mnist_images(
		data_dir_path + "/t10k-images.idx3-ubyte",
		&test_images,
		-1.0, // remap 0 to -1.0
		1.0,  // remap 255 to 1.0
		2,    // add two pixels of padding horizontally
		2     // add two pixels of padding vertically
	);

	// Setup the optimizer and a progress display.
	std::cout << "Training started.\n";
	tiny_dnn::adagrad optimizer;
	tiny_dnn::progress_display disp(train_images.size());
	tiny_dnn::timer t;

	// Scale the learning rate.
	optimizer.alpha *= std::min(
		tiny_dnn::float_t(4),
		static_cast<tiny_dnn::float_t>(sqrt(n_minibatch) * learning_rate)
	);

	// Create a callback that upon finishing an epoch, will display details on
	// the progress, time elapsed, and how well the test images are recognized.
	int epoch = 1;
	auto on_enumerate_epoch = [&]() {
		std::cout
			<< "Epoch " << epoch << "/" << n_train_epochs << " finished. "
			<< t.elapsed() << "s elapsed."
			<< "\n";

		// Perform a test run.
		tiny_dnn::result res = nn.test(test_images, test_labels);
		std::cout
			<< res.num_success << "/" << res.num_total
			<< " (" << ((float)res.num_success / res.num_total * 100)
			<< "%) recognized.\n";

		// Increment the epoch and restart the display and timer.
		++epoch;
		disp.restart(train_images.size());
		t.restart();
	};

	// Create a callback that upon finishing a minibatch, will update the
	// progress display.
	auto on_enumerate_minibatch = [&]() { disp += n_minibatch; };

	// Train the network.
	nn.train<tiny_dnn::mse>(
		optimizer,
		train_images,
		train_labels,
		n_minibatch,
		n_train_epochs,
		on_enumerate_minibatch,
		on_enumerate_epoch
	);
	std::cout << "Training ended.\n";

	// Run a test and dump the results.
	nn.test(test_images, test_labels).print_detail(std::cout);

	// Save the trained network.
	nn.save("mnist");
}


static void
usage(const char *argv0) {
	std::cerr
	<< "usage: " << argv0
	<< " --data <data_dir>"
	<< " --rate 1"
	<< " --epochs 30"
	<< " --minibatch 16"
	<< " --backend internal"
	<< "\n";
}


int main(int argc, char **argv) {
	// Setup some default parameters.
	double learning_rate = 1;
	int epochs = 30;
	std::string data_path = "";
	int minibatch_size = 16;
	tiny_dnn::core::backend_t backend_type = tiny_dnn::core::default_engine();

	// Parse the command line arguments.
	if (argc == 2) {
		std::string argname(argv[1]);
		if (argname == "--help" || argname == "-h") {
			usage(argv[0]);
			return 0;
		}
	}

	for (int count = 1; count + 1 < argc; count += 2) {
		std::string argname(argv[count]);
		if (argname == "--rate") {
			learning_rate = atof(argv[count + 1]);
		} else if (argname == "--epochs") {
			epochs = atoi(argv[count + 1]);
		} else if (argname == "--minibatch") {
			minibatch_size = atoi(argv[count + 1]);
		} else if (argname == "--backend") {
			backend_type = parse_backend_name(argv[count + 1]);
		} else if (argname == "--data") {
			data_path = std::string(argv[count + 1]);
		} else {
			std::cerr << "unknown parameter \"" << argname << "\"\n";
			usage(argv[0]);
			return 1;
		}
	}

	// Verify that the parameters are sane.
	if (data_path == "") {
		std::cerr << "Data path not specified.\n";
		usage(argv[0]);
		return 1;
	}

	if (learning_rate <= 0) {
		std::cerr << "Invalid learning rate. The learning rate must be greater than 0.\n";
		return 1;
	}

	if (epochs <= 0) {
		std::cerr << "Invalid number of epochs. The number of epochs must be greater than 0.\n";
		return 1;
	}

	if (minibatch_size <= 0 || minibatch_size > 60000) {
		std::cerr << "Invalid minibatch size. The minibatch size must be greater than 0 and less than dataset size (60000).\n";
		return 1;
	}

	// Dump the parameters we finally use for training.
	std::cout << "Running with the following parameters:\n"
		<< "Data path: " << data_path << "\n"
		<< "Learning rate: " << learning_rate << "\n"
		<< "Minibatch size: " << minibatch_size << "\n"
		<< "Number of epochs: " << epochs << "\n"
		<< "Backend type: " << backend_type << "\n"
		<< "\n";

	// Actually train the network.
	try {
		train_lenet(data_path, learning_rate, epochs, minibatch_size, backend_type);
	} catch (tiny_dnn::nn_error &err) {
		std::cerr << "Exception: " << err.what() << "\n";
		return 1;
	}
}

