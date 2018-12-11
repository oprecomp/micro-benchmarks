#!/bin/bash -e
# Copyright (c) 2017 OPRECOMP Project

CSTAT=`tput bold`
CERR=`tput bold``tput setaf 1`
CSUCC=`tput bold``tput setaf 2`
CRST=`tput sgr0`

# Root Directory of all MBs.
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "ROOT=$ROOT"

RUNDIR="BaseLine_000"

run()
{
	### IDLE 
	$ROOT/idle/BenchmarkRunner.sh $RUNDIR mkdirs
	$ROOT/idle/BenchmarkRunner.sh $RUNDIR run
	$ROOT/idle/BenchmarkRunner.sh $RUNDIR parse

	### BLSTM
	$ROOT/blstm/BenchmarkRunner.sh $RUNDIR mkdirs
	$ROOT/blstm/BenchmarkRunner.sh $RUNDIR run
	$ROOT/blstm/BenchmarkRunner.sh $RUNDIR parse

	### PAGERANK
	$ROOT/pagerank/BenchmarkRunner.sh $RUNDIR mkdirs
	$ROOT/pagerank/BenchmarkRunner.sh $RUNDIR run
	$ROOT/pagerank/BenchmarkRunner.sh $RUNDIR parse

	### GLQ
	$ROOT/glq/BenchmarkRunner.sh $RUNDIR mkdirs
	$ROOT/glq/BenchmarkRunner.sh $RUNDIR run
	$ROOT/glq/BenchmarkRunner.sh $RUNDIR parse
}

plot()
{
	$ROOT/idle/BenchmarkRunner.sh $RUNDIR plot
	$ROOT/blstm/BenchmarkRunner.sh $RUNDIR plot
	$ROOT/pagerank/BenchmarkRunner.sh $RUNDIR plot
	$ROOT/glq/BenchmarkRunner.sh $RUNDIR plot
}

useage()
{
   echo "Usage: [run|plot]" >&2
}


if [ "$#" -ne 1 ]; then
	useage
	exit 1
fi

case "$1" in
	run)    run   "${@:2}";;
	plot)   plot  "${@:2}";;
	*)		useage;;
esac
