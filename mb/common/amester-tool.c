// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

// This is a simple program that uses the `AmesterMeasurements` facility to run
// a measurement between two characters read from standard input. This allows
// for non-C benchmarks to be measured by starting this tool as a subprocess and
// issuing the start/stop commands to its standard input.

#include "oprecomp.h"
#include <stdio.h>


int
main (int argc, char **argv) {
	int c;

	// Wait for the '{' character that indicates the beginning of the
	// measurement.
	do {
		c = getc(stdin);
	} while (c != '{');
	oprecomp_start();

	// Wait for the '}' character that indicates the end of the measurement.
	// The '.' character is also accepted and will be translated to a call to
	// oprecomp_iterate, whose return value is printed to stdout.
	do {
		c = getc(stdin);
		if (c == '.') {
			putc(oprecomp_iterate() ? '1' : '0', stdout);
			putc('\n', stdout);
			fflush(stdout);
		}
	} while (c != '}');
	oprecomp_stop();

	return 0;
}
