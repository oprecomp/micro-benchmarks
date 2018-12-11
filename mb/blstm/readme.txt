Additional Notes:
	- the directory gt/ is moved due the large file-sizes that do not belong in the src-code reporsitory.
	- the directory paper/ is deleted from that source directory for the same reason, it can be found in the *.zip checked in the oprecomp-data repository.

Make sure that you have the following setup:

1. Check a version of a 'cmake' to build the project
$ cmake --version
cmake version 2.6 or higher

2. Check a version of a 'make' to build the executable 
$ g++ --version
g++ 4.8 or higher supporting C++11

3. Make sure that you have the following stucture of the project directory
$ tree
.
├── alphabet
│   └── alphabet.txt
├── build
├── CMake_includes
│   └── compiler_options.cmake
├── CMakeLists.txt
├── gt
│   ├── fontane_brandenburg01_1862_0043_1600px_010001.gt.txt
.	.
.	.
.	.
│   └── fontane_brandenburg04_1882_0459_1600px_010033.gt.txt
├── include
│   └── neuron.hpp
├── model
│   ├── model_bw.txt
│   └── model_fw.txt
├── readme.txt
└── src
    ├── main.cpp
    └── neuron.cpp

4. Navigate to a build folder 
$ cd build/

5. Clean the folder. Build the project
$ rm -rf *
$ cmake ..

6. Build the executable 
$ make

7. Choose a single (-s) thread or a multiple (-m) threads implementation. Point to a folder with the dataset. Execute.
*
* The Fraktur dataset was orogonally used in http://ieeexplore.ieee.org/abstract/document/7333935/?reload=true
*
$ ./main -m ../../fraktur_dataset/samples/ or $ ./main -s ../../fraktur_dataset/samples/

8. The sample output:

Number of threads on the current machine: 8
Host name: rvv-HP-ProBook-650-G1
Start ...
Accuracy: 98.2337%
Measured time ... 43 seconds
