#!/bin/bash
# Copyright (c) 2017-2018 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"

for batch_size in 32 64 128 256; do
	echo "# running batch_size=$batch_size" >&2
	export CUDA_VISIBLE_DEVICES=
	"$BMDIR/baseline/tf-learn.py" --batch-size $batch_size --num-steps 1000 --no-save
done
