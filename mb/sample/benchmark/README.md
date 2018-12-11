# Benchmark Sample

This is a sample microbenchmark that attempts to collect performance data. The benchmark is in `main.c` and is quite simple: Some meaningless floating point operations on a data set. The size of the data set (i.e. the problem size) and the number of threads crunching it are passed to the benchmark via arguments:

    ./sample <problem_size> <num_threads>

Steps to adjust your benchmark:

1. Include header
2. Add common directory to include directories searched by compiler
3. Add `oprecomp_start`/`oprecomp_stop`
4. Optionally add `do { ... } while (oprecomp_iterate());`
5. Add `benchmark.sh` script and run your benchmarks there


## Instrumenting Your Code

To instrument your code, include the OPRECOMP header:

    #include "oprecomp.h"

and add the `mb/common` directory to your include path. Together with your benchmark, also compile the `mb/common/oprecomp.c` file. In your code, annotate the region you want to measure as follows:

    oprecomp_start();
    // ... your kernel code
    oprecomp_stop();

Make sure to not include initialization in between start and stop. Since the benchmark needs to run for some reasonable amount of time for the measurements to be meaningful, you can optionally make use of the `oprecomp_iterate()`, as follows:

    oprecomp_start();
    do {
        // ... your kernel code
    } while (oprecomp_iterate());
    oprecomp_stop();

The `oprecomp_iterate()` function will count the number of times your kernel was executed, and will return 1 as long as the total execution time is too short to allow proper power measurements.


## Collecting Data

The `oprecomp.{h,c}` files provide a transparent framework to measure your kernels. If you execute your benchmark, you will see additional lines emitted to stdout:

    # time_total: ...
    # iterations: ...
    # time_avg: ...

To measure the performance of your benchmark, don't run it directly but rather pass the command that you would use to run it to the `mb/common/measure.py` script, as follows:

    # original benchmark call
    ./sample <problem_size> <num_threads>

    # performance measurements done as follows
    <path_to_common>/measure.py ./sample <problem_size> <num_threads>

Simply prefix the command with the path to the `measure.py` script. The script will execute your benchmark and collect the measurement results. It also parses your benchmark's standard output and collects all lines of the form:

    # <header>: <value>

These lines, together with the power measurement results, are concatenated into one line of CSV data, preceded by a CSV header explaining the fields. The header line is prefixed with `#` such that it can easily be filtered out of the output.


## Performing Parameter Sweeps

Your benchmark should provide an executable `benchmark.sh` file. In this script, you should run your benchmark for every desired parameter combination. It is advisable that your benchmark accepts its parameters as regular program arguments (`argc/argv`), and emits its configuration as `# <param>: <value>` lines to stdout. Take a look at `mb/sample/benchmark/benchmark.sh` for an example. The output of the script is valid CSV, and can easily be redirected to a file. From there it can be post-processed, rearranged, or plotted directly. You might want to remove the header lines with grep.


## Configuring AMESTER

The measurement script will coordinate an AMESTER power measurement if the following environment variables are set:

- `AMESTER_ENABLE = 1`: Execute AMESTER power measurement
- `AMESTER_HOST` (optional): The host running AMESTER
- `AMESTER_PORT` (optional): The port on which AMESTER is listening
- `AMESTER_CONFIG` (optional): The configuration to use
- `AMESTER_FILE` (optional): File on the remote server where data is logged
