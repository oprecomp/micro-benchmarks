#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

ARG0="$0"
ROOT="$(dirname "${BASH_SOURCE[0]}")"
BENCHMARKS="\
	sample/shell \
	sample/make \
	sample/cmake \
	sample/gtest \
	sample/data \
	sample/benchmark \
	pagerank \
	kmeans \
	knn \
	svm/baseline \
	lstm \
	blstm \
	pca \
	cnn \
	gd \
	glq \
	fft \
	stencil \
	sparsesolve \
"

CSTAT=`tput bold`
CERR=`tput bold``tput setaf 1`
CSUCC=`tput bold``tput setaf 2`
CRST=`tput sgr0`

# Determine the hash command to use.
case `uname` in
	Darwin) SHASUM="shasum";;
	Linux) SHASUM="sha1sum";;
	*)
		echo "cannot determine SHA1 hash command for uname `uname`" >&2
		;;
esac

# Create a temporary directory for log files.
TMP="/tmp/oprecomp-$USER"
if [ ! -d "$TMP" ]; then
	mkdir "$TMP"
fi

# source the machine dependent configuration if it exists
if [ -e "$ROOT/config.sh" ]; then
	source "$ROOT/config.sh"
fi

print_usage() {
	echo "usage: benchmarks.sh (-h|--help)" >&2
	echo "   or: benchmarks.sh (BENCHMARK|-a|--all)" >&2
	echo "   or: benchmarks.sh prepare (BENCHMARK|-a|--all)" >&2
	echo "   or: benchmarks.sh build (BENCHMARK|-a|--all)" >&2
	echo "   or: benchmarks.sh test (BENCHMARK|-a|--all)" >&2
	echo "   or: benchmarks.sh clean [BENCHMARK]" >&2
	echo "   or: benchmarks.sh update-data" >&2
}


setup_build_dir() {
	# Make sure the benchmark actually exists.
	if [ ! -d "$2" ]; then
		echo "benchmark \"$1\" does not exist" >&2
		exit 1
	fi

	# Create a build directory for the benchmark.
	if [ ! -d "$2/build" ]; then
		mkdir "$2/build"
	fi

	# Create a symlink to the data directory if none exists.
	if [ ! -e "$2/build/data" ]; then
		ln -sf "$PWD/$ROOT/data" "$2/build/data"
	fi
}


# Executes the given arguments as a command and ensures that the working
# directory is restored to the status quo after the command returns.
recover_pwd_after() {
	local DIR="$PWD"
	"$@"
	local STATUS=$?
	cd "$DIR"
	return $STATUS
}


# The prepare_benchmark function is called once for every benchmark if the
# source data has changed, in order to extract/prepare the input data for the
# benchmark.
prepare_benchmark() {
	local BMDIR="$ROOT/$1"

	# Make sure the benchmark actually exists.
	if [ ! -d "$BMDIR" ]; then
		echo "benchmark \"$1\" does not exist" >&2
		return 1
	fi

	# Create a build directory for the benchmark.
	setup_build_dir "$1" "$BMDIR" || return

	# Create the "prepared" data folder if it does not yet exist.
	if [ ! -d "$ROOT/data/prepared" ]; then
		mkdir "$ROOT/data/prepared" || return
	fi

	# In order to be able to detect changes in the source data such that we can
	# determine whether to rerun the preparation script or not, every benchmark
	# needs a hash file that contains a fingerprint of the source data that it
	# has processed. These hashes are put inside the "data/prepared/.hashes"
	# directory, which is created if it does not yet exist. The name of the hash
	# file is the benchmark name, with "/" replaced by "_".
	HASH_PATH="$ROOT/data/prepared/.hashes"
	if [ ! -d "$HASH_PATH" ]; then
		mkdir "$HASH_PATH" || return
	fi
	HASH_PATH="$HASH_PATH/${1//\//_}"

	# Check whether the source data changed, in which case the preparation is
	# necessary. Otherwise return early and skip the preparation part.

	# Read the fingerprint of the source data that we have seen during the last
	# run of the prepare script. Otherwise just assume an empty string, which
	# will always fail to match the current state of the repo.
	if [ -e "$HASH_PATH" ]; then
		local HASH_SEEN=`cat $HASH_PATH`
	fi

	# Read the SHA1 revision of the oprecomp-data repository. This will be the
	# first part of the current fingerprint.
	pushd "$ROOT/data/source" > /dev/null
	HASH_NOW=`git rev-parse HEAD`
	popd > /dev/null

	# Add the hash of the prepare.sh script, if it exists.
	if [ -e "$BMDIR/prepare.sh" ]; then
		HASH_NOW="$HASH_NOW:"`$SHASUM "$BMDIR/prepare.sh" | cut -d " " -f 1`
	fi

	# Add the hashes of other files listed in the prepare.files file, if it
	# exists.
	if [ -e "$BMDIR/prepare.files" ]; then
		HASH_NOW="$HASH_NOW:"`$SHASUM "$BMDIR/prepare.files" | cut -d " " -f 1`
		while read -r filename; do
			if [ ! -z "$filename" ]; then
				HASH_NOW="$HASH_NOW:"`$SHASUM "$BMDIR/$filename" | cut -d " " -f 1`
			fi
		done < "$BMDIR/prepare.files"
	fi

	# If the current hash agrees with the previous one, return and skip the
	# preparation.
	if [ "$HASH_SEEN" == "$HASH_NOW" ]; then
		return 0
	fi

	if [ -e "$BMDIR/prepare.sh" ]; then
		if [ ! -x "$BMDIR/prepare.sh" ]; then
			echo "prepare script $BMDIR/prepare.sh is not executable" >&2
			echo "did you forget to \`chmod +x $BMDIR/prepare.sh\`?" >&2
			return 1
		fi
		pushd "$BMDIR/build" > /dev/null || return
		../prepare.sh "${@:2}" || return
		popd > /dev/null || return
	fi

	# Store the current hash.
	echo "$HASH_NOW" > "$HASH_PATH"
}


# The build_benchmark function is called once for every benchmark to be built.
build_benchmark() {
	local BMDIR="$ROOT/$1"

	# Make sure the benchmark actually exists.
	if [ ! -d "$BMDIR" ]; then
		echo "benchmark \"$1\" does not exist" >&2
		return 1
	fi

	# Create a build directory for the benchmark.
	setup_build_dir "$1" "$BMDIR" || return

	# Based on what files are given in the benchmark directory, decide how to
	# perform the build. The following is supported:

	# Shell script
	if [ -e "$BMDIR/build.sh" ]; then
		if [ ! -x "$BMDIR/build.sh" ]; then
			echo "build script $BMDIR/build.sh is not executable" >&2
			echo "did you forget to \`chmod +x $BMDIR/build.sh\`?" >&2
			return 1
		fi
		cd "$BMDIR/build" || return
		../build.sh "${@:2}" || return

	# Makefile
	elif [ -e "$BMDIR/Makefile" ]; then
		cd "$BMDIR/build" || return
		make -f ../Makefile build "${@:2}" || return

	# CMake
	elif [ -e "$BMDIR/CMakeLists.txt" ]; then
		cd "$BMDIR/build" || return
		cmake .. "${@:2}" || return
		make || return

	else
		echo "don't know how to build benchmark $1" >&2
		echo "missing either build.sh, Makefile, or CMakeLists.txt" >&2
		return 1
	fi
}


test_benchmark() {
	local BMDIR="$ROOT/$1"

	# Make sure the benchmark and its build directory actually exist.
	if [ ! -d "$BMDIR" ]; then
		echo "benchmark \"$1\" does not exist" >&2
		return 1
	fi
	if [ ! -d "$BMDIR/build" ]; then
		echo "benchmark \"$1\" has not been built (the build directory is missing)" >&2
		echo "did you forget to call either" >&2
		echo "- \`$ARG0 build -a\`, or" >&2
		echo "- \`$ARG0 build $1\`?" >&2
		return 1
	fi

	# Based on what files are given in the benchmark directory, decide how to
	# perform the test. The following is supported:

	# Shell script
	if [ -e "$BMDIR/test.sh" ]; then
		if [ ! -x "$BMDIR/test.sh" ]; then
			echo "test script $BMDIR/test.sh is not executable" >&2
			echo "did you forget to \`chmod +x $BMDIR/test.sh\`?" >&2
			return 1
		fi
		cd "$BMDIR/build" || return
		../test.sh "${@:2}" || return

	# Makefile
	elif [ -e "$BMDIR/Makefile" ]; then
		cd "$BMDIR/build" || return
		make -f ../Makefile test "${@:2}" || return

	# CMake
	elif [ -e "$BMDIR/CMakeLists.txt" ]; then
		cd "$BMDIR/build" || return
		make test || return

	else
		echo "don't know how to test benchmark $1" >&2
		echo "missing either test.sh, Makefile, or CMakeLists.txt" >&2
		return 1
	fi
}


# The following function pulls the latest source data from the remote
# repository.
update_data() {
	# Create the data directory if it does not yet exist.
	if [ ! -d "$ROOT/data" ]; then
		mkdir "$ROOT/data" || return
	fi

	# Clone the data repository if it does not yet exist. If it does, simply
	# fetch the most recent changes.
	if [ ! -d "$ROOT/data/source" ]; then
		echo "${CSTAT}cloning${CRST} oprecomp-data repository"
		git clone git@iis-git.ee.ethz.ch:oprecomp/oprecomp-data.git "$ROOT/data/source" || return
	fi

	# Check wether the current HEAD is equivalent origin/master before the
	# fetch, which indicates that the repository has not been touched or
	# committed to since the last checkout. If these are equivalent, perform the
	# fetch, and fast-forward.
	cd "$ROOT/data/source"
	REV_HEAD=`git rev-parse HEAD`
	REV_ORIGIN=`git rev-parse origin/master`
	if [ "$REV_HEAD" == "$REV_ORIGIN" ]; then
		git fetch origin || return
		git merge --quiet --ff-only origin/master || return
	else
		echo "not updating data/source due to local changes; update manually" >&2
	fi
}


# The following function handles the "prepare" subcommand.
handle_prepare() {
	case "$1" in
		-a|--all)
			# Prepare all existing benchmarks.
			local RUNSET="$BENCHMARKS"
			local DONT_CHECK=false
			;;
		"")
			echo "no benchmark name or options given" >&2
			print_usage
			exit 1
			;;
		*)
			# Prepare specific benchmarks.
			local RUNSET="$@"
			local DONT_CHECK=true
			;;
	esac

	# Fetch the data if needed.
	if [ ! -z "$RUNSET" ]; then
		if ! recover_pwd_after update_data; then
			echo "failed to update data" >&2
			return 1
		fi
	fi

	# Prepare the benchmarks and log the outputs.
	local NUM_FAILED=0
	for bm in $RUNSET; do
		if $DONT_CHECK || [ -d "$ROOT/$bm" ]; then
			local LOGPATH="$TMP/prepare_${1//\//_}"
			echo "${CSTAT}preparing${CRST} $bm"
			if ! recover_pwd_after prepare_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}prepare failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
			fi
		fi
	done

	# Produce a summary in case of failed prepares.
	if [ $NUM_FAILED -gt 0 ]; then
		echo "${CERR}$NUM_FAILED prepares failed${CRST}" >&2
	fi
	return $NUM_FAILED
}


# The following function handles invocations of the form:
# `benchmarks.sh build ...`
handle_build() {
	case "$1" in
		-a|--all)
			# Build all existing benchmarks.
			local RUNSET="$BENCHMARKS"
			local DONT_CHECK=false
			;;
		"")
			echo "no benchmark name or options given" >&2
			print_usage
			exit 1
			;;
		*)
			# Build specific benchmarks.
			local RUNSET="$@"
			local DONT_CHECK=true
			;;
	esac

	# Fetch the data if needed.
	if [ ! -z "$RUNSET" ]; then
		if ! recover_pwd_after update_data; then
			echo "failed to update data" >&2
			return 1
		fi
	fi

	# Build the benchmarks and log the outputs.
	local NUM_FAILED=0
	for bm in $RUNSET; do
		if $DONT_CHECK || [ -d "$ROOT/$bm" ]; then
			local LOGPATH="$TMP/build_${1//\//_}"
			echo "${CSTAT}building${CRST} $bm"
			if ! recover_pwd_after build_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}build failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
			fi
		fi
	done

	# Produce a summary in case of failed builds.
	if [ $NUM_FAILED -gt 0 ]; then
		echo "${CERR}$NUM_FAILED builds failed${CRST}" >&2
	fi
	return $NUM_FAILED
}


# The following function handles invocations of the form:
# `benchmarks.sh test ...`
handle_test() {
	case "$1" in
		-a|--all)
			# Test all existing benchmarks.
			local RUNSET="$BENCHMARKS"
			local DONT_CHECK=false
			;;
		"")
			echo "no benchmark name or options given" >&2
			print_usage
			exit 1
			;;
		*)
			# Test specific benchmarks.
			local RUNSET="$@"
			local DONT_CHECK=true
			;;
	esac

	# Fetch the data if needed.
	if [ ! -z "$RUNSET" ]; then
		if ! recover_pwd_after update_data; then
			echo "failed to update data" >&2
			return 1
		fi
	fi

	# Test the benchmarks and log the outputs.
	local NUM_FAILED=0
	for bm in $RUNSET; do
		if $DONT_CHECK || [ -d "$ROOT/$bm" ]; then
			local LOGPATH="$TMP/test_${1//\//_}"
			echo "${CSTAT}testing${CRST} $bm"
			if ! recover_pwd_after test_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}test failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
			fi
		fi
	done

	# Produce a summary in case of failed tests.
	if [ $NUM_FAILED -gt 0 ]; then
		echo "${CERR}$NUM_FAILED tests failed${CRST}" >&2
	fi
	return $NUM_FAILED
}


# The following function handles the "clean" subcommand.
handle_clean() {
	# Remove all the build directories.
	for bm in $BENCHMARKS; do
		if [ -d "$ROOT/$bm/build" ]; then
			rm -r "$ROOT/$bm/build"
		fi
	done
}


# The following function handles the "update-data" subcommand.
handle_update_data() {
	recover_pwd_after update_data
}


# The following function handles invocations that are neither build nor test.
handle_default() {
	case "$1" in
		-a|--all)
			local RUNSET="$BENCHMARKS"
			local DONT_CHECK=false
			;;
		"")
			echo "no subcommand or options given" >&2
			print_usage
			exit 1
			;;
		*)
			local RUNSET="$@"
			local DONT_CHECK=true
			;;
	esac

	# Fetch the data if needed.
	if [ ! -z "$RUNSET" ]; then
		if ! recover_pwd_after update_data; then
			echo "failed to update data" >&2
			return 1
		fi
	fi

	# Build the benchmarks and log the outputs.
	local NUM_TOTAL=0
	local NUM_FAILED=0
	for bm in $RUNSET; do
		if $DONT_CHECK || [ -d "$ROOT/$bm" ]; then
			echo "${CSTAT}running${CRST} $bm"
			((NUM_TOTAL += 1))

			# Prepare
			local LOGPATH="$TMP/prepare_${1//\//_}"
			if ! recover_pwd_after prepare_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}prepare failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
				continue
			fi

			# Build
			local LOGPATH="$TMP/build_${1//\//_}"
			if ! recover_pwd_after build_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}build failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
				continue
			fi

			# Test
			local LOGPATH="$TMP/test_${1//\//_}"
			if ! recover_pwd_after test_benchmark $bm &> "$LOGPATH"; then
				cat "$LOGPATH" >&2
				echo "${CERR}test failed${CRST} for $bm" >&2
				((NUM_FAILED += 1))
				continue
			fi
		fi
	done

	# Produce a summary in case of failed tests.
	echo >&2
	if [ $NUM_FAILED -gt 0 ]; then
		echo "  finished: ${CERR}$NUM_FAILED/$NUM_TOTAL benchmarks failed${CRST}" >&2
	else
		echo "  finished: ${CSUCC}$NUM_TOTAL benchmarks passed${CRST}" >&2
	fi
	echo >&2
	return $NUM_FAILED
}


# If the user supplied a -h or --help switch somewhere among the arguments,
# print the usage/help page and exit.
for arg in "$@"; do
	if [ "$arg" = "-h" ] || [ "$arg" = "--help" ]; then
		print_usage
		exit
	fi
done

case "$1" in
	prepare)     handle_prepare     "${@:2}";;
	build)       handle_build       "${@:2}";;
	test)        handle_test        "${@:2}";;
	clean)       handle_clean       "${@:2}";;
	update-data) handle_update_data "${@:2}";;
	*)           handle_default     "${@:1}";;
esac
