#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project

CSTAT=`tput bold`
CERR=`tput bold``tput setaf 1`
CSUCC=`tput bold``tput setaf 2`
CRST=`tput sgr0`

# Directory of current MB.
BMDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

FILEPREFIX="KernelMeasure_"
BENCHMARK_PROG="$BMDIR/build/benchmark/myBenchmarkRunner"
BENCHMARK_PARSER="$ROOT/pagerank/build/benchmark/BenchmarkResultParser" # use the same tool
BENCHMARK_PLOTTER="$ROOT/pagerank/benchmark/TimePowerEnergyPlotter.py" 	# use the same tool

RUNDIR=$1

### Folder structure
# MB - RunDir(Algo Version) - Measure / Results / Plots

LOCATION="/fl/eid/DATA/glq"
OUTPREFIX="$LOCATION/$RUNDIR/Measure/$FILEPREFIX"
OUTPREFIX_PATH="$LOCATION/$RUNDIR/Measure"
OUTPREFIX_PATH_RES="$LOCATION/$RUNDIR/Results"
OUTPREFIX_PATH_PLOT="$LOCATION/$RUNDIR/Plots"


mkdirs()
{
	mkdir $LOCATION/$RUNDIR
	mkdir $LOCATION/$RUNDIR/Measure
	mkdir $LOCATION/$RUNDIR/Results
	mkdir $LOCATION/$RUNDIR/Plots
}

#############################################################################
### 1) RUN THE MEASUREMENT 
#############################################################################
run()
{
	pushd $BMDIR/build
	rm -rf tmp
	mkdir tmp
	pushd tmp
	ln -s ../data data
	echo "RUNNING: $BENCHMARK_PROG $OUTPREFIX"
	$BENCHMARK_PROG $OUTPREFIX
	popd
	echo "cp tmp/* $OUTPREFIX_PATH_RES"
	cp tmp/* $OUTPREFIX_PATH_RES
	popd
}

#############################################################################
### 2) Parse the Data
#############################################################################
parse()
{
	echo "PARSING: $BENCHMARK_PARSER -parse $OUTPREFIX $OUTPREFIX_PATH/LongOut.csv"
	$BENCHMARK_PARSER -parse $OUTPREFIX $OUTPREFIX_PATH/LongOut.csv
}

#############################################################################
### 3) Generate Data and Plots
#############################################################################
plot()
{
	pushd $BMDIR/build
	source $ROOT/configPowerMeasurement.sh
	echo "GETTING DATA: scp $DOWNLOAD_DATA_FROM_HOST:$OUTPREFIX_PATH/LongOut.csv LongOut.csv"
	scp $DOWNLOAD_DATA_FROM_HOST:$OUTPREFIX_PATH/LongOut.csv LongOut.csv
	rm -rf out
	mkdir out
	echo "PLOTTING: python3.6 $BENCHMARK_PLOTTER LongOut.csv out/"
	python3.6 $BENCHMARK_PLOTTER LongOut.csv out/
	echo "scp out/* $DOWNLOAD_DATA_FROM_HOST:$OUTPREFIX_PATH_PLOT"
	scp out/* $DOWNLOAD_DATA_FROM_HOST:$OUTPREFIX_PATH_PLOT
	echo "SCRIPT DONE"
	popd 
}

useage()
{
   echo "Usage: $0 <RunDir> [mkdirs|run|parse|plot]" >&2
}

if [ "$#" -ne 2 ]; then
	useage
	exit 1
fi

case "$2" in
	mkdirs) mkdirs   "${@:2}";;
	run)    run      "${@:2}";;
	parse)	parse 	 "${@:2}";;
	plot)	plot 	 "${@:2}";;
	*)		useage;;
esac

