add_executable( BenchmarkResultParser BenchmarkResultParser.cpp)
target_link_libraries (BenchmarkResultParser lib)

add_executable( myBenchmarkRunner myBenchmarkRunner.cpp )
target_link_libraries (myBenchmarkRunner lib)