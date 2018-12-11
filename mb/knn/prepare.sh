#!/bin/bash -e

SRCDIR="data/source/mb/knn"
OUTDIR="data/prepared/mb/knn"

# Let's create a directory for our prepared data.
mkdir -p "$OUTDIR"

gzip -d -c "$SRCDIR/adult.data.gz" > "$OUTDIR/adult.data"
