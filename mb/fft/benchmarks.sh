#!/bin/bash
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

export AMESTER_DURATION=10
for i in 1 2 3 4 5 6 7 8 9 10
do
n=$(( 1000000*i ))
echo $n
$MEASURE ./run_fftw $n
done
