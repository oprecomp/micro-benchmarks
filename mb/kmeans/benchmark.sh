#!/bin/bash
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

for num_threads in 1 2 4 8 16; do
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -i data/source/mb/kmeans/color100.txt
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -i data/source/mb/kmeans/edge100.txt
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -i data/source/mb/kmeans/texture100.txt
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -b -i data/prepared/mb/kmeans/color17695.bin
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -b -i data/prepared/mb/kmeans/edge17695.bin
    "$MEASURE" ./omp_main -o -n 4 -p $num_threads -b -i data/prepared/mb/kmeans/texture17695.bin
done
