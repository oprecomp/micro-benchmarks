#!/usr/bin/env python3
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This is the wrapper script for performing performance measurements on
# individual microbenchmarks. Simply prefix your benchmark execution with this
# script to obtain results:
#
#     measure.py my_benchmark [BENCHMARK_ARGUMENTS...]
#
# Lines starting with "# oprecomp_result: " are expected to contain valid CSV
# data and are concatenated together with the timing and power measurements into
# a single line of CSV output. This allows multiple benchmarks to be run, the
# results of which form a CSV table of results.

import sys
import os
import subprocess
import re

DIR = os.path.dirname(os.path.realpath(sys.argv[0]))

if len(sys.argv) < 2:
	sys.stderr.write("usage: %s BENCHMARK_PROGRAM [BENCHMARK_ARGUMENTS...]\n" % sys.argv[0])
	sys.exit(1)

sys.stderr.write("measuring %s\n" % " ".join(sys.argv[1:]))

# Execute the benchmark and gather the result lines.
output = subprocess.check_output(sys.argv[1:], universal_newlines=True)
results = [(r.group(1), r.group(2)) for r in re.finditer('^#\s*(.*)\s*:\s*(.*)\s*$', output, flags=re.MULTILINE)]

# Read back the data measured by AMESTER, or some dummy values.
if os.getenv("AMESTER_ENABLE", "0") == "1":
	amester_header = subprocess.check_output(["ssh", os.environ.get("AMESTER_HOST"), "cat", os.environ.get("AMESTER_FILE") + "_header.csv"], universal_newlines=True)
	amester_data = subprocess.check_output(["ssh", os.environ.get("AMESTER_HOST"), "cat", os.environ.get("AMESTER_FILE") + "_data.csv"], universal_newlines=True)
else:
	with open(DIR+"/measure_header.csv", "r") as f:
		amester_header = f.read()
	with open(DIR+"/measure_data.csv", "r") as f:
		amester_data = f.read()

# Split the AMESTER header and the two data lines into their fields.
amester_header = [f.strip().strip("::mysys_") for f in amester_header.split("\n")[0].split(",")]
amester_data = [[f.strip() for f in l.split(",")] for l in amester_data.split("\n")[:2]]

# Calculate the sensor values accumulated and divide by the time taken to do so.
# Each header corresponds to three data fields: t, acc, up
sensors = list()
for i,h in enumerate(amester_header):
	acc1 = float(amester_data[0][i*3+1])
	acc2 = float(amester_data[1][i*3+1])
	up1 = float(amester_data[0][i*3+2])
	up2 = float(amester_data[1][i*3+2])
	sensors.append((h, "%f" % ((acc2-acc1)/(up2-up1))))

# Emit the data as two lines of CSV: one header and one data line. The header
# line is prefixed with a '#' for convenience.
concat = results + sensors
print("#", ", ".join([v[0] for v in concat]))
print(", ".join([v[1] for v in concat]))
