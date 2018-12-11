#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This script is called from within a temporary build directory. However, by
# convention, that directory contains a `data/source` subdirectory which
# corresponds to the oprecomp-data repository, and a `data/prepared`
# subdirectory which shall hold the prepared data.

SRCDIR="data/source/mb/sample/data"
OUTDIR="data/prepared/mb/sample/data"

# Let's create a directory for our prepared data.
mkdir -p "$OUTDIR"

# For example, we can decompress an archive from the `data/source` directory
# into our own `data/prepared/mb/sample/data` directory.
tar -C "$OUTDIR" -xzf "$SRCDIR/input.tar.gz"

# Now that we have the uncompressed data, we might want to preprocess it, for
# example by sorting the lines.
sort < "$OUTDIR/input.txt" > "$OUTDIR/sorted.txt"
