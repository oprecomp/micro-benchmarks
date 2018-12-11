#!/bin/bash -e

SRCDIR="data/source/mb/sparsesolve"
OUTDIR="data/prepared/mb/sparsesolve"

# Let's create a directory for our prepared data.
mkdir -p "$OUTDIR"

gzip -d -c "$SRCDIR/bcsstk01.mtx.gz" > "$OUTDIR/bcsstk01.mtx"
gzip -d -c "$SRCDIR/gr_30_30.mtx.gz" > "$OUTDIR/gr_30_30.mtx"
gzip -d -c "$SRCDIR/msc10848.mtx.gz" > "$OUTDIR/msc10848.mtx"
