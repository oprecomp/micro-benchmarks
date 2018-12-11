# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# When this makefile is called, the current working directory will be some form
# of temporary build directory. Use the following line to get the path to the
# microbenchmark directory:
BMDIR = $(dir $(firstword $(MAKEFILE_LIST)))

# The build target is used to compile the executables. You can have multiple
# executables here, e.g. for different precisions or algorithm architectures. At
# your discretion.
build: sample_make

# The test target is used to execute the unit tests for this microbenchmark.
# Make sure that your benchmark produces the expected results.
test: sample_make
	./sample_make

sample_make: $(BMDIR)/main.cpp
	$(CXX) -O3 -o $@ $^
