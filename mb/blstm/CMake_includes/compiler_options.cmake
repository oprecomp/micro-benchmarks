#
# Copyright (C) 2017 University of Kaiserslautern
# Microelectronic Systems Design Research Group
#
# Vladimir Rybalkin (rybalkin@eit.uni-kl.de)
# 20. February 2017
#



# Compiler and linker options for gcc
if(CMAKE_COMPILER_IS_GNUCXX)

  # C code compiler
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Wall -Wextra -O3")
  #set(CMAKE_C_FLAGS_DEBUG "-ggdb3")
  #set(CMAKE_C_FLAGS_PROF "-pg -O3")
  
  # C++ code compiler
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -std=c++11")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -std=c++11")
  #set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -std=c++11")
  #set(CMAKE_CXX_FLAGS_PROF "-pg -O3 -std=c++11")
  
  # Linker flags
  #set(CMAKE_EXE_LINKER_FLAGS "")
  #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
  #set(CMAKE_EXE_LINKER_FLAGS_PROF "-pg")

endif(CMAKE_COMPILER_IS_GNUCXX)
