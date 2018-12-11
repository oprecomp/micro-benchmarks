
cmake_minimum_required (VERSION 2.6)
project (idle)

add_definitions(-std=c++11)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    
    message("-- OPENMP_FOUND is ${OPENMP_FOUND}")
    message("-- CMAKE_CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
endif()

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
