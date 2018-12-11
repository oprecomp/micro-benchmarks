#!/usr/bin/env python3
# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

# This script takes a CSV file which contains header lines beginning with a '#'
# and properly tabularizes the data, inserting empty fields where necessary.

import sys
import os
import subprocess
import re
import csv

csv_regex = re.compile('\s*,\s*')
sticky_regex = re.compile('^(.*?)\s*:\s*(.*?)$')

# Parse the standard input as CSV. Keep a list of fields that were encountered,
# and convert the data into a key-value pair mapping.
current_header = None
field_list = list()
field_set = set()
sticky_data = dict()
data = list()
for line in sys.stdin.readlines():
	line = line.strip()
	if len(line) == 0:
		continue
	if line[0] == "#":
		line = line[1:].strip()
		sticky = sticky_regex.match(line)
		if sticky is not None:
			f = sticky.group(1)
			sticky_data[f] = sticky.group(2)
			if not f in field_set:
				field_list.append(f)
				field_set.add(f)
		else:
			current_header = csv_regex.split(line)
			for f in current_header:
				if not f in field_set:
					field_list.append(f)
					field_set.add(f)
	else:
		data.append(dict(list(sticky_data.items()) + list(zip(current_header, csv_regex.split(line)))))

# for row in csv.reader(sys.stdin):
# 	row = [f.strip() for f in row]
# 	if row[0][0] == "#":
# 		current_header = [row[0].strip("#").strip()] + row[1:]
# 		for f in current_header:
# 			if not f in field_set:
# 				field_list.append(f)
# 				field_set.add(f)
# 	else:
# 		data.append(dict(zip(current_header, row)))

# Emit one line of CSV for each datum, inserting empty fields if the datum does
# not provide it.
print("#", ", ".join(field_list))
for datum in data:
	print(", ".join([datum[f] if f in datum else "" for f in field_list]))
