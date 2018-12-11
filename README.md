# OPRECOMP Micro-Benchmarks

This repository contains the micro-benchmarks used throughout the [OPRECOMP project](https://github.com/oprecomp/oprecomp) to gauge the impact of transprecision techniques on a broad range of applications.



## Contents

Each of the following benchmarks is located in the `mb/` directory and represents a computation kernel found throughout the fields of Big Data, Deep Neural Networks, and Scientific Computing.

### Deep Learning
| Benchmark | Leader | CPU | GPU | FPGA | PULP |
|-----------|--------|-----|-----|------|------|
| CNN       | ETH    | ✔   | ✔   |      | ✔    |
| GD        | ETH    |     | ✔   |      |      |
| BLSTM     | IBM    | ✔   | ✔   | ✔    | ✔    |
| PCA       | UJI    | ✔   |     |      |      |

### Big Data and Data Analytics
| Benchmark | Leader | CPU | GPU | FPGA | PULP |
|-----------|--------|-----|-----|------|------|
| PageRank  | IBM    | ✔   | ✔   |      |      |
| K-means   | UJI    | ✔   |     |      |      |
| KNN       | UJI    | ✔   |     |      |      |
| SVM       | CINECA | ✔   | ✔   |      |      |

### High Performance Computing (HPC) and Scientific Computing
| Benchmark | Leader | CPU | GPU | FPGA | PULP |
|-----------|--------|-----|-----|------|------|
| GLQ       | IBM    | ✔   | ✔   |      |      |
| FFT       | CINECA | ✔   |     |      |      |
| Stencil   | CINECA | ✔   |     |      |      |
| SparseSolve | UJI  | ✔   |     |      |      |



## Developing Microbenchmarks

To ensure that the microbenchmarks merged into the upstream repository are stable, do compile, and are error-free, we employ continuous integration to build and run the benchmarks whenever they are pushed to a fork on GitLab or the upstream repository. In order to do this, and to make the process of compiling and running the benchmarks as uniform as possible, we propose the setup outline below.

The `benchmarks.sh` script builds and executes the benchmarks. Currently execution only involves unit tests to ensure the tests are error-free and perform as expected, but we may wish to extend this later to also cover performance comparisons. The script can be run from within any working directory, allowing you to call it from within your benchmark or any other location. Use the script as follows:

- `./benchmarks.sh pagerank` runs (prepares, builds, and tests) the *pagerank* benchmark
- `./benchmarks.sh --all` runs all benchmarks; this happens on the CI server
- `./benchmarks.sh prepare pagerank` prepares the *pagerank* benchmark
- `./benchmarks.sh build pagerank` builds the *pagerank* benchmark
- `./benchmarks.sh test pagerank` tests the *pagerank* benchmark

As mentioned above, the benchmark lifecycle is currently split into the steps *prepare*, *build*, and *test*, in this order. These are outlined in the following sections, together with an explanation of the means by which you can perform each of the steps. The convention is that each of the steps is executed in the same temporary build directory.

**Note on current working directory:** To ensure that the repository remains clean, we require that the prepare, build, and test steps can be executed out-of-source. This means that it must be possible to call your scripts, Makefiles, or CMake setup, from an arbitrary working directory. Outputs of these scripts must go into that working directory. See `sample/{shell,make,cmake}` for examples on how to reliably determine the path to your benchmark.


### prepare

The *prepare* step is intended for data preparation. Many of the benchmarks will require a set of input data for testing and later performance comparison. To keep data use limited, we strongly encourage to place the input data in its compressed, original form into the `data/source` directory. See the section on separation of input data below. The preparation step is intended for unpacking and reformatting the input data from `data/source/<benchmark>` into `data/prepared/<benchmark>`. For example, the *pagerank* benchmark untars its `All.tar` file and converts the input data with a separate program.

During this step, the `prepare.sh` file in your benchmark is executed. In this script, assume that there is a `data/source` directory where the contents of the *oprecomp-data* repository are available, and that there is a `data/prepared` directory where your unpacked and reformatted input data shall be placed.

See `sample/prepare` for an example.


### build

The *build* step is intended for compiling the different variants of your benchmark. For example, you might want to compile your benchmark for fp32, fp16, and fp8 separately. Our setup supports Makefiles, CMake, and custom shell scripts for this step, as explained in the next sections. The build step is executed in the same directory as the prepare step, so you have access to all temporaries you created during preparation.

#### Shell Script

This is the most generic option. If your benchmark provides a `build.sh` file, that file is executed within the build directory. See the note above about the current working directory. See `sample/shell/build.sh` for an example.

#### Make

Providing a makefile is preferred over a custom build script. If no build script is found and your benchmark provides a `Makefile`, we call `make build` on it from within the build directory. See the note above about the current working directory. See `sample/make/Makefile` for an example.

#### CMake

Providing a CMake configuration is preferred over a makefile. If no makefile is found and your benchmark provides a `CMakeLists.txt`, we call `cmake` on your benchmark directory and then `make` from within the build directory. See the note above about the current working directory. See `sample/cmake/CMakeLists.txt` for an example.


### test

The *test* step is intended for executing the programs you compiled above, in order to execute unit tests and to ensure that they are functioning properly. We expect every benchmark to at least contain some basic sanity checks as to whether it functions properly. If you find bugs, consider adding a regression test that would fail if the bug was to be ever re-introduced. Note that we would like to postpone performance comparison and benchmarking to a separate step. Our setup supports Makefiles, CMake, and custom shell scripts for this step, as explained in the next sections. The test step is executed in the same directory as the build step, so you have access to all build artifacts and temporaries you created during preparation.

#### Shell Script

This is the most generic option. If your benchmark provides a `test.sh` file, all other files are ignored and that file is executed within the build directory. See the note above about the current working directory. See `sample/shell/test.sh` for an example.

#### Make

Providing a makefile is preferred over a custom test script. If no test script is found and your benchmark provides a `Makefile`, we call `make test` on it from within the build directory. See the note above about the current working directory. See `sample/make/Makefile` for an example.

#### CMake

Providing a CMake configuration is preferred over a makefile. If no makefile is found and your benchmark provides a `CMakeLists.txt`, we call `make test` from within the build directory. See the note above about the current working directory. See `sample/cmake/CMakeLists.txt` for an example.



## Separation of Input Data

In order to keep the `oprecomp` repository focused on source code and reports rather than on input data, we propose to split all binary or large input data sets into the separate `oprecomp-data` repository. Large means more than 4 KiB. The data repository is automatically cloned whenever you execute the prepare step of one of the benchmarks. To manually clone it, call `./benchmarks.sh update-data`. This is almost never required, though.

The data repository is cloned to `data/source`. It is also made available in the build directories of each benchmark, as `data/source`. To add data to this repository, either

- send them to us via email; or
- fork the `oprecomp-data` repository, add the data into your own `mb/<benchmark>` directory, and open a merge request on GitLab



## Open Questions

- What do we do with dependencies like gtest and tiny-dnn? Is each user responsible for installing them, and the `../ci` directory provides scripts to do this step on the continuous integration servers? Or should we rather have the `benchmarks.sh` script ensure that the dependencies are installed locally?


 The micro-benchmarks are structured in three damains: Deep learning, big data and high performance computing and they cover the following kernels:



## License

The source code in this repository is licensed under the terms of the Apache License (Version 2.0), unless otherwise specified. See [LICENSE](LICENSE) for details.
