########################################################################
### PROJECT OPRECOMP 												 ###
###------------------------------------------------------------------###
### Purpose:	THIS IS THE ROOT FILE FOR COMPILATION  				 ###
### 			COMPILATION WORKS AS FOLLOWS: 						 ###
### 			1) create a empty folder 							 ###
### 			2) go into that folder  							 ###
###				3) call CMAKE by pointing to that root file 		 ###
### 			4) call make to produce the project 				 ### 		
###				USE THE FOLLOWING: 									 ###	
###				mkdir build 										 ###
###				cd build 											 ###
### 			cmake ../ 											 ###
### 			make 												 ###
### INFO ABOUT CMAKE: [1] 											 ###
### INFO ABOUT GTEST: [2] 											 ###	
###------------------------------------------------------------------###
### PROJECT MANAGEMENT:			GIT RESPONSIBLE: 					 ###
### ----------------------		-----------------------------------  ###
### Cristiano Malossi		 	1.) 	Fabian Schuiki		         ###
### IBM Research - Zurich 				ETH Zurich 					 ###
### Research Staff Member 	 			fschuiki@iis.ee.ethz.ch  	 ###
### acm@zurich.ibm.com    		2.) 	Florian Scheidegger 		 ###
### +41(0)44 724 8616					IBM Research - Zurich  		 ###
### 										eid@zurich.ibm.com 		 ###
########################################################################
# [1]: https://cmake.org/cmake-tutorial/ 
# [2]: https://www.ibm.com/developerworks/aix/library/au-googletestingframework.html#list1
# [3]: https://github.com/google/googletest

## GTEST
### git clone https://github.com/google/googletest.git
### cmake cmake CMAKE file
### make

### HOW TO https://cmake.org/pipermail/cmake/2006-October/011641.html

# CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ cmake ..
# CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ CMAKE_EXE_LINKER_FLAGS="-qq" cmake ..
# 

## THIS WORKS 
## SEE: http://stechschulte.net/2016/03/20/openmp-osx-cmake.html (no additional stuff required, with find_package lines)
# CC=clang-omp CXX=clang-omp++ cmake ..

#CC 		= /usr/local/opt/llvm/bin/clang
#CPP 		= /usr/local/opt/llvm/bin/clang++

#set (OpenMP_C_FLAGS "-L/usr/local/opt/llvm/lib")
#set (OpenMP_CXX_FLAGS "-L/usr/local/opt/llvm/lib")

#set (OpenMP_C_FLAGS "-I/usr/local/opt/llvm/include -fopenmp")
#set (OpenMP_CXX_FLAGS "FFFF")
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} FFFFFF")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")


cmake_minimum_required (VERSION 2.8.2)
# cmake_minimum_required (VERSION 3.0.2)

project (OPRECOMP)

# The version number.
set (MB_VERSION_MAJOR 1)
set (MB_VERSION_MINOR 0)

# configure a header file to pass the CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/MBConfig.h.in"
  "${PROJECT_BINARY_DIR}/MBConfig.h"
  )

add_definitions(-std=c++11)



message("Checking: OPRECOMP_NONDEFAULT_GTEST_LIB:" $ENV{OPRECOMP_NONDEFAULT_GTEST_LIB})
if(DEFINED ENV{OPRECOMP_NONDEFAULT_GTEST_LIB})
    message(STATUS "OPRECOMP_NONDEFAULT_GTEST_LIB environment variable defined")

	# GTEST LIBRARY
	find_library(LIB_LOCATION_GTEST gtest)
	message("==> " ${LIB_LOCATION_GTEST})
	add_library(gtest SHARED IMPORTED)
	set_target_properties(gtest
	   PROPERTIES IMPORTED_LOCATION ${LIB_LOCATION_GTEST})
else()
	message("==> looking for default gtest")
endif()



message("Checking: OPRECOMP_HAS_CUDA:" $ENV{OPRECOMP_HAS_CUDA})
if(DEFINED ENV{OPRECOMP_HAS_CUDA})
    message(STATUS "OPRECOMP_HAS_CUDA environment variable defined")
    # some more commands
    find_package(CUDA)
	if(CUDA_FOUND)
			# REGULAR CUDA CASE
			message("GPU CUDA SUPPORTED")
			add_definitions(-DGPU)
			list(APPEND CUDA_NVCC_FLAGS "-std=c++11")
			# on most systems: /usr/local/cuda/samples/common
			set (CUDA_SDK_ROOT_DIR "${CUDA_TOOLKIT_ROOT_DIR}/samples/common")
			message("==> " ${CUDA_SDK_ROOT_DIR})
	else()
		message("GPU CUDA *** NOT *** SUPPORTED")
	endif()
else()
    message(STATUS "OPRECOMP_HAS_CUDA environment variable NOT defined")
endif()

find_package(OpenMP)
if (OPENMP_FOUND)
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_LAGS} ${OpenMP_CXX_FLAGS}")
endif()



set( MY_LINK_FLAGS    "-fopenmp")
set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${MY_LINK_FLAGS}" )

#############################################################################
### Define compiler flags.
#############################################################################
# CALL CMAKE WIHT CORRECT SWITCH
# cmake -DCMAKE_BUILD_TYPE=OURFLAGSA ..
# cmake -DCMAKE_BUILD_TYPE=OURFLAGSB ..
# OR HARD-CODE
# SET(CMAKE_BUILD_TYPE OURFLAGSA)


# NOTES:
# The following flags work on the power machine, namely on
# SCLIGERA etc....
# They do not work on grizzly.

cmake_host_system_information(RESULT MY_HOSTNAME QUERY HOSTNAME)
message("===>" ${MY_HOSTNAME} "<===")

if(CMAKE_BUILD_TYPE)
	message("CMAKE_BUILD_TYPE is defined: ${CMAKE_BUILD_TYPE} ")	
	if(	(${CMAKE_BUILD_TYPE} STREQUAL "OURFLAGSGCCA") OR
		(${CMAKE_BUILD_TYPE} STREQUAL "OURFLAGSGCCB") OR
		(${CMAKE_BUILD_TYPE} STREQUAL "OURFLAGSXLCA") OR
                (${CMAKE_BUILD_TYPE} STREQUAL "OURFLAGSXLCB") OR
                (${CMAKE_BUILD_TYPE} STREQUAL "OURFLAGSXLCC") )
		message("OWN FLAGS ARE FOUND.")
		if(${MY_HOSTNAME} STREQUAL "grizzly")
			message(FATAL_ERROR " You are on EID's personal computer [grizzly], which is not a POWER8 architecture and does not support the requested flags.")
		endif()
	else()
		message(WARNING "You are using BUILD type: " ${CMAKE_BUILD_TYPE} )
	endif()
endif()

# THE FOLLOWING WORK WITH GCC (tested with 5.3.1)
# VERSION A OF FLAGS
SET(CMAKE_C_FLAGS_OURFLAGSGCCA 		"-O2 -mcpu=power8 -mtune=power8 -m64")
SET(CMAKE_CXX_FLAGS_OURFLAGSGCCA 	"-O2 -mcpu=power8 -mtune=power8 -m64")

# VERSION B OF FLAGS
SET(CMAKE_C_FLAGS_OURFLAGSGCCB 		"-O3 -mcpu=power8 -mtune=power8 -m64 -ftree-vectorize -ffast-math -mveclibabi=mass -fopt-info")
SET(CMAKE_CXX_FLAGS_OURFLAGSGCCB 	"-O3 -mcpu=power8 -mtune=power8 -m64 -ftree-vectorize -ffast-math -mveclibabi=mass -fopt-info")


# THE FOLLOWING WORK WITH XLC (tested with 13.01.0005.0000)
# VERSION A OF FLAGS
SET(CMAKE_C_FLAGS_OURFLAGSXLCA 		"-O2 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8")
SET(CMAKE_CXX_FLAGS_OURFLAGSXLCA 	"-O2 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8")

# VERSION B OF FLAGS
SET(CMAKE_C_FLAGS_OURFLAGSXLCB 		"-O3 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8 -qinline -qsmp=omp -qthreaded -qsimd=auto -qhot -lmass -lmassv -lmass_simdp8")
SET(CMAKE_CXX_FLAGS_OURFLAGSXLCB 	"-O3 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8 -qinline -qsmp=omp -qthreaded -qsimd=auto -qhot -lmass -lmassv -lmass_simdp8")

# FULL REF: https://www.ibm.com/support/knowledgecenter/SSXVZZ_13.1.5/com.ibm.compilers.linux.doc/welcome.html

# Then, with XL you can push up to -O5. Behind -O4 and -O5 there is -qipa, which does a lot of things. -qipa=level=2 is the max
# -qnostrict this is very important: typically when you add it codes run much faster, but it is less safe (do not strictly respect IEE-745)

# VERSION C OF FLAGS
SET(CMAKE_C_FLAGS_OURFLAGSXLCC          "-O5 -qipa=level=2 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8 -qinline -qsmp=omp -qthreaded -qsimd=auto -qhot=level=1 -qprefetch=aggressive:dscr=7 -lmass -lmassv -lmass_simdp8 -qnostrict")
SET(CMAKE_CXX_FLAGS_OURFLAGSXLCC        "-O5 -qipa=level=2 -qmaxmem=-1 -q64 -qarch=pwr8 -qtune=pwr8 -qinline -qsmp=omp -qthreaded -qsimd=auto -qhot=level=1 -qprefetch=aggressive:dscr=7 -lmass -lmassv -lmass_simdp8 -qnostrict")

### Location of CMAKE-generated *.h files
include_directories("${PROJECT_BINARY_DIR}")
#############################################################################
### MB common code
### 	- Amester measurement instrumentalization
###		- BenchmarkRunner
#############################################################################
#message(INFO "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
include_directories ("${PROJECT_SOURCE_DIR}/../../common/clientapi/")
include_directories ("${PROJECT_SOURCE_DIR}/../../common/util/")

#############################################################################
### util (contains helper functions to print, debug and load/store data)
#############################################################################
include_directories ("${PROJECT_SOURCE_DIR}/util")

#############################################################################
### Include library with reference kernels
#############################################################################
include_directories ("${PROJECT_SOURCE_DIR}/lib")
add_subdirectory (lib)

#############################################################################
### BUILDING TESTSUITE
#########st(APPEND CUDA_NVCC_FLAGS "-std=c++11")
####################################################################
add_subdirectory (testsuite)

#############################################################################
### BUILDING BENCHMARK
#############################################################################
add_subdirectory (benchmark)

