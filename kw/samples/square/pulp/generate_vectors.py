#!/usr/bin/env python3
# Generate test vectors for `square`.

import struct
import os
import random
import sys

DIR = os.path.abspath(os.path.dirname(sys.argv[0]))
ADDR_BASE = (1 << 47)
NUM_WORDS = 1000
DATA_SIZE = NUM_WORDS * 8

wed = struct.pack("QQQ", NUM_WORDS, 24+ADDR_BASE, 24+DATA_SIZE+ADDR_BASE)
inputs = list()
output_stm = list()
output_exp = list()
for i in range(NUM_WORDS):
	v = random.randint(-2**31, 2**31-1)
	inputs.append(struct.pack("q", v))
	output_stm.append(struct.pack("q", 0))
	output_exp.append(struct.pack("q", v*v))

with open(os.path.join(DIR, "square_stm.vec"), "wb") as veci:
	veci.write(wed)
	for d in inputs:
		veci.write(d)
	for d in output_stm:
		veci.write(d)

with open(os.path.join(DIR, "square_exp.vec"), "wb") as veco:
	veco.write(wed)
	for d in inputs:
		veco.write(d)
	for d in output_exp:
		veco.write(d)
