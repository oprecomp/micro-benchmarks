#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This script installs the gtest library.

ROOT="$(dirname "${BASH_SOURCE[0]}")"
TMP="$HOME/.tmp/oprecomp"
SRCDIR="$TMP/src"

# Create the temporary directory if it does not yet exist.
mkdir -p "$SRCDIR"

# Download and build the gtest sources if needed.
GTEST_VERSION="1.8.0"
GTEST_SRC="$SRCDIR/gtest-$GTEST_VERSION"

if [ ! -e "$GTEST_SRC.tar.gz" ]; then
	echo "Downloading gtest"
	curl -L "https://github.com/google/googletest/archive/release-$GTEST_VERSION.tar.gz" > "$GTEST_SRC.tar.gz"
fi

# Extract gtest and move it into place.
if [ ! -d "$GTEST_SRC" ]; then
	echo "Extracting gtest"
	tar -C "$SRCDIR" -xzf "$GTEST_SRC.tar.gz"
	mv "$SRCDIR/googletest-release-$GTEST_VERSION" "$GTEST_SRC"

	# Build and install gtest.
	echo "Building gtest"
	pushd "$GTEST_SRC" > /dev/null
	mkdir build
	cd build
	cmake -Dgtest_build_samples=OFF -DCMAKE_INSTALL_PREFIX="$TMP" ..
	make -j
	echo "Installing gtest"
	make install
	popd > /dev/null
fi
