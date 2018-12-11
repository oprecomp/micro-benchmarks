#!/bin/bash
# Copyright (c) 2017-2018 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"

for batch_size in 1 2 4 8 16 32 64 128 256; do
	echo "# running batch_size=$batch_size" >&2
	export CUDA_VISIBLE_DEVICES=0,1
	"$BMDIR/baseline/tf-infer.py" --batch-size $batch_size
done
