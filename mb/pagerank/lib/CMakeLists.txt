if (CUDA_FOUND)
	cuda_add_library(lib kernels.cpp kernels.cu)
else()
	add_library(lib kernels.cpp)
endif()


