#!/bin/bash -e

SRCDIR="data/source/mb/kmeans"
OUTDIR="data/prepared/mb/kmeans"

# Let's create a directory for our prepared data.
mkdir -p "$OUTDIR"

gzip -d -c "$SRCDIR/color17695.bin.gz"   > "$OUTDIR/color17695.bin"
gzip -d -c "$SRCDIR/edge17695.bin.gz"    > "$OUTDIR/edge17695.bin"
gzip -d -c "$SRCDIR/texture17695.bin.gz" > "$OUTDIR/texture17695.bin"
