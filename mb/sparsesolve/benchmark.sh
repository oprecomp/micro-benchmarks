#!/bin/bash

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

for num_threads in 1 ; do
        "$MEASURE" ./sparsesolve data/prepared/mb/sparsesolve/bcsstk01.mtx 10000 1e-7
        "$MEASURE" ./sparsesolve data/prepared/mb/sparsesolve/gr_30_30.mtx 10000 1e-7
        "$MEASURE" ./sparsesolve data/prepared/mb/sparsesolve/msc10848.mtx 10000 1e-7
done

