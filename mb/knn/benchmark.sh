#!/bin/bash

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

for num_threads in 1 ; do
    "$MEASURE" ./knn 10 data/prepared/mb/knn/adult.data 1000
done

