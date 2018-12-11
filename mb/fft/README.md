# FFT microbenchmark for Oprecomp

1. Description
2. Running the microbenchmark
3. TODO

## Description

This microbenchmark uses an FFT micro-kernel taken from the Quantum
Espresso program and based on **FFTW v2**. 
Currently the kernel does only a very simple 1 D complex array
initilalisation, performs a forward transform and then a reverse
transform, and then compares the result.

## Running the microbenchmark

Since the array is initialised within the program no external data are
required. It is sufficient to call just the executbale or the
```test.sh``` script (which at the moment just calls the executable).
``` shell
./run_fftw
```

## TODO

Current plans for improvements include:
1. Add timing routine
2. More sophisticated test case requiring more time, further FFT
   functionality and perhaps based on a scientific/engineering
   example.
