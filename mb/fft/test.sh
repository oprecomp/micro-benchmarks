#!/bin/bash
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

for i in 1000000 5000000 10000000
do
$MEASURE ./run_fftw $i
done
