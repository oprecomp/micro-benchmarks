#!/bin/bash
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

# Make sure the amester-tool is compiled. This is only needed for this specific
# benchmark, due to it being written in Python.
make -f "$BMDIR/Makefile" amester-tool >/dev/null

for batch_size in 1 2 4 8 16 32 64 128 256; do
	echo "# running batch_size=$batch_size" >&2
	"$MEASURE" "$BMDIR/tf-infer.py" --batch-size $batch_size --amester ./amester-tool
done
