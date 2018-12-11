#!/bin/bash -e

BMDIR="$(dirname "${BASH_SOURCE[0]}")"
SRCDIR="data/source/mb/gd"
OUTDIR="data/prepared/mb/gd"

# Let's create a directory for our prepared data.
mkdir -p "$OUTDIR"

[ -e "$OUTDIR/train-images.idx3-ubyte" ] || gzip -d -c "$SRCDIR/train-images-idx3-ubyte.gz" > "$OUTDIR/train-images.idx3-ubyte"
[ -e "$OUTDIR/train-labels.idx1-ubyte" ] || gzip -d -c "$SRCDIR/train-labels-idx1-ubyte.gz" > "$OUTDIR/train-labels.idx1-ubyte"

[ -e "$OUTDIR/t10k-images.idx3-ubyte" ] || gzip -d -c "$SRCDIR/t10k-images-idx3-ubyte.gz" > "$OUTDIR/t10k-images.idx3-ubyte"
[ -e "$OUTDIR/t10k-labels.idx1-ubyte" ] || gzip -d -c "$SRCDIR/t10k-labels-idx1-ubyte.gz" > "$OUTDIR/t10k-labels.idx1-ubyte"

[ -d "$OUTDIR/cifar-10-batches-bin" ] || tar -xzv -C "$OUTDIR" -f "$SRCDIR/cifar-10-binary.tar.gz"

if [ ! -e "$OUTDIR/cifar-10-train.data.bin" ] || [ ! -e "$OUTDIR/cifar-10-train.labels.bin" ]; then
	echo "augmenting training data"
	"$BMDIR/baseline/prepare_cifar10.py" --distort "$OUTDIR/cifar-10-batches-bin"/data_batch_*.bin -o "$OUTDIR/cifar-10-train" -m 20
fi

if [ ! -e "$OUTDIR/cifar-10-test.data.bin" ] || [ ! -e "$OUTDIR/cifar-10-test.labels.bin" ]; then
	echo "preprocessing testing data"
	"$BMDIR/baseline/prepare_cifar10.py" "$OUTDIR/cifar-10-batches-bin/test_batch.bin" -o "$OUTDIR/cifar-10-test"
fi
