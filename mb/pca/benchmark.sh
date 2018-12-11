#!/bin/bash

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

for problem_size in 9; do
    for num_threads in 1 2 4 8 16; do
        "$MEASURE" ./test_pca $problem_size $num_threads
    done
done
