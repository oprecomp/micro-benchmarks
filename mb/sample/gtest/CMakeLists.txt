# Copyright (c) 2017 OPRECOMP Project
# Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
cmake_minimum_required(VERSION 2.8)
project(gtest_sample)

add_definitions(-std=c++11)

add_executable(gtest_sample main.cpp)
target_link_libraries(gtest_sample gtest)

enable_testing()
add_test(my_test gtest_sample)
