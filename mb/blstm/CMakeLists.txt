#
# Copyright (C) 2017 University of Kaiserslautern
# Microelectronic Systems Design Research Group
#
# Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
# 20. February 2017
#



cmake_minimum_required (VERSION 2.6)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)
project (bigdata.nn.ocr)

#message("CMAKE_CURRENT_SOURCE_DIR is ${CMAKE_CURRENT_SOURCE_DIR}")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_definitions(-std=c++11)

include_directories(include)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake_includes")
include(compiler_options)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    
    message("-- OPENMP_FOUND is ${OPENMP_FOUND}")
    message("-- CMAKE_CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
endif()

add_library(lib src/neuron.cpp)

add_executable(main src/main.cpp)
target_link_libraries( main lib )

#############################################################################
### MB common code
### 	- Amester measurement instrumentalization
###		- BenchmarkRunner
#############################################################################
include_directories ("${PROJECT_SOURCE_DIR}/../../common/clientapi/")
include_directories ("${PROJECT_SOURCE_DIR}/../../common/util/")

#############################################################################
### BUILDING BENCHMARK
#############################################################################
add_subdirectory (benchmark)

