#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# When this script is called, the current working directory will be some form of
# temporary build directory. Use the following line to get the path to the
# microbenchmark directory:
BMDIR="$(dirname "${BASH_SOURCE[0]}")"

# Quickly check that the data we have sorted during the prepare step is the same
# as what we expect. This is a trivial example, and data testing might not be
# necessary in your benchmark. However, you will want to feed the prepared data
# into your tests.
diff -q "data/prepared/mb/sample/data/sorted.txt" "$BMDIR/expected.txt"
