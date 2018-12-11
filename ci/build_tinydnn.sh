#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This script installs the tiny-dnn library.

ROOT="$(dirname "${BASH_SOURCE[0]}")"
TMP="$HOME/.tmp/oprecomp"
SRCDIR="$TMP/src"

# Create the temporary directory if it does not yet exist.
mkdir -p "$SRCDIR"

# Download and build the gtest sources if needed.
TINYDNN_VERSION="1.0.0a3"
TINYDNN_SRC="$SRCDIR/tiny-dnn-$TINYDNN_VERSION"

# Download the source.
if [ ! -e "$TINYDNN_SRC.tar.gz" ]; then
	echo "Downloading tiny-dnn"
	curl -L "https://github.com/tiny-dnn/tiny-dnn/archive/v$TINYDNN_VERSION.tar.gz" > "$TINYDNN_SRC.tar.gz"
fi

# Extract tiny-dnn and move it into place.
if [ ! -d "$TINYDNN_SRC" ]; then
	echo "Extracting tiny-dnn"
	tar -C "$SRCDIR" -xzf "$TINYDNN_SRC.tar.gz"
fi

# Copy the headers to the appropriate directory.
mkdir -p "$TMP/include"
rsync -ah --delete "$TINYDNN_SRC/tiny_dnn" "$TINYDNN_SRC/cereal" "$TINYDNN_SRC/third_party" "$TMP/include/"
