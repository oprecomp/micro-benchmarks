#!/bin/bash
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This is a sample benchmark script. Use this as a foundation for your own
# scripts. The concept is quite easy: Run your benchmark once with each
# parameter variation you want to test. In the example, the benchmark takes two
# arguments: the problem size and the number of threads. Instead of directly
# running the benchmark, call it through the measurement script, which will
# perform power measurements as appropriate.
#
# Your benchmark can emit arbitrary standard output. For final result gathering,
# everything is ignored except for lines of the shape:
#
#     # <header>: <value>
#
# These are extracted and concatenated together with the measured execution time
# and power consumption. Note that dummy power values will be returned if you
# don't have an AMESTER setup configured.

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../../common/measure.py"

for problem_size in 22 23 24 25 26; do
	for num_threads in 1 2 4 8 16; do
		"$MEASURE" ./sample $problem_size $num_threads
	done
done
