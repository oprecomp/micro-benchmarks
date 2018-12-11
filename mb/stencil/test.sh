#!/bin/bash
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"
for grid in 200 300 400; do
   for nthd in 1 2 4; do
      $MEASURE ./jacobi $grid $grid $nthd
      #./jacobi $grid $grid $nthd
   done
done
