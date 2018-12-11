# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
cmake_minimum_required(VERSION 2.8)
project(cmake_sample)

# Use ${CMAKE_CURRENT_SOURCE_DIR} to access the benchmark directory. This is
# useful for fetching small unit test input data.

# Use ${CMAKE_CURRENT_BINARY_DIR} to access the build directory. This is useful
# for fetching large sets of input data from the oprecomp-data repository, as
# follows: "${CMAKE_CURRENT_BINARY_DIR}/data/prepared/<benchmark>/bigdata.csv"

# The following line enables C++11 support. This is likely the highest we can
# go.
add_definitions(-std=c++11)

# Compile an executable. You can have multiple of these, e.g. for different
# precisions or algorithm architectures. At your discretion.
add_executable(cmake_sample main.cpp)

# Add unit tests for this benchmark. Use this to test the executables compiled
# above, or if you use a unit testing framework, to execute those tests.
enable_testing()
add_test(simple_test cmake_sample)
