#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# When this script is called, the current working directory will be some form of
# temporary build directory. Use the following line to get the path to the
# microbenchmark directory:
BMDIR="$(dirname "${BASH_SOURCE[0]}")"

g++ -O3 -o shell_sample $BMDIR/main.cpp
