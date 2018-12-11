#!/bin/bash
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

set -e
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../common/measure.py"

# Make sure the amester-tool is compiled. This is only needed for this specific
# benchmark, due to it being written in Python.
make -f "$BMDIR/Makefile" amester-tool >/dev/null

for device in cpu gpu; do
	echo "# device: $device"
	for batch_size in 32 64 128 256; do
		echo "# running device=$device batch_size=$batch_size" >&2
		case $device in
			cpu) prefix="env CUDA_VISIBLE_DEVICES=";;
			gpu) prefix="env CUDA_VISIBLE_DEVICES=0,1";;
		esac
		$prefix "$MEASURE" "$BMDIR/baseline/tf-learn.py" --batch-size $batch_size --num-steps 1000 --no-save --no-eval --amester ./amester-tool
	done
done
