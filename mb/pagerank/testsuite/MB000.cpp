#include <cmath>
#include <stdint.h>

#include <omp.h>
#include <sched.h>

#include <gtest/gtest.h>
#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"

#include <ctime>
#include <chrono>

double avg( std::vector<double> &vec)
{
    double sum = 0;
    for(size_t i = 0; i<vec.size(); ++i)
    {
        sum += vec[i];
    }
    return sum / ((double ) vec.size());
}

double stdev( std::vector<double> &vec)
{
    double mu = avg(vec);
    double sum = 0;
    for(size_t i = 0; i<vec.size(); ++i)
    {
        sum += (mu - vec[i])*(mu - vec[i]);
    }
    sum = sum / ((double ) vec.size() - 1);
    return sqrt(sum);
}

// global arguments to access command line options.
int my_argc;
char** my_argv;

// ########################################################################
// CHECKING DATATYPES
// ########################################################################

// Ok, most systems have 32/64 bit float and double types. But just to make sure.
TEST (SystemFloatTest, SizeOfDataType ) { 
    EXPECT_EQ (4, sizeof(float) ); 		// 32 bit
    EXPECT_EQ (8, sizeof(double) ); 	// 64 bit
}

// That is pretty standard and works on most systems
TEST (SystemIntTest, SizeOfDefault_DataType ) { 
    EXPECT_EQ (2, sizeof(short) ); 			// 16 bit
    EXPECT_EQ (4, sizeof(int) ); 			// 32 bit
    EXPECT_EQ (8, sizeof(long int) ); 		// 64 bit
    EXPECT_EQ (8, sizeof(long long int) );  // 64 bit as well
}

// Please note, that is the way to code:
TEST (SystemIntTest, SizeOfSafe_DataType ) { 
    EXPECT_EQ (1, sizeof(int8_t) ); 		// 8 bit
    EXPECT_EQ (2, sizeof(int16_t) ); 		// 16 bit
    EXPECT_EQ (4, sizeof(int32_t) );  		// 32 bit
    EXPECT_EQ (8, sizeof(int64_t) ); 		// 64 bit
}

// ########################################################################
// CHECKING UTILITY IO ROUTINES
// ########################################################################

template<typename T> 
void checkFloat000(const std::vector<std::vector<T> > &Matrix) 
{
    EXPECT_FLOAT_EQ (0.0, Matrix[0][0]);
    EXPECT_FLOAT_EQ (0.0, Matrix[0][1]);
    EXPECT_FLOAT_EQ (1.0, Matrix[0][2]);
    EXPECT_FLOAT_EQ (0.0, Matrix[0][3]);

    EXPECT_FLOAT_EQ (0.0, Matrix[1][0]);
    EXPECT_FLOAT_EQ (0.0, Matrix[1][1]);
    EXPECT_FLOAT_EQ (0.0, Matrix[1][2]);
    EXPECT_FLOAT_EQ (1.0, Matrix[1][3]);

    EXPECT_FLOAT_EQ (0.0, Matrix[2][0]);
    EXPECT_FLOAT_EQ (0.0, Matrix[2][1]);
    EXPECT_FLOAT_EQ (0.0, Matrix[2][2]);
    EXPECT_FLOAT_EQ (0.0, Matrix[2][3]);

    EXPECT_FLOAT_EQ (0.125, Matrix[3][0]);
    EXPECT_FLOAT_EQ (0.5, Matrix[3][1]);
    EXPECT_FLOAT_EQ (0.0, Matrix[3][2]);
    EXPECT_FLOAT_EQ (0.0, Matrix[3][3]);
}

void checkInt000( const std::vector<std::vector<int> > &Matrix )
{
    EXPECT_EQ (1, Matrix[0][0]);
    EXPECT_EQ (2, Matrix[0][1]);
    EXPECT_EQ (3, Matrix[0][2]);
    EXPECT_EQ (4, Matrix[0][3]);

    EXPECT_EQ (100, Matrix[1][0]);
    EXPECT_EQ (200, Matrix[1][1]);
    EXPECT_EQ (300, Matrix[1][2]);
    EXPECT_EQ (400, Matrix[1][3]);

    EXPECT_EQ (1000, Matrix[2][0]);
    EXPECT_EQ (2000, Matrix[2][1]);
    EXPECT_EQ (3000, Matrix[2][2]);
    EXPECT_EQ (4000, Matrix[2][3]);
}

TEST (Util_IO, floatIn ) { 
    std::vector<std::vector<double> > Matrix;
    Matrix = readCSVMatrix<double>( std::string(my_argv[1]) + std::string( "/float_in000.csv"), ',');
    PrintMatrix<double>( Matrix );
    PrintMatrix<double>( Matrix, 0, 2, 0, 2 );
    printf("------------------------------------\n");
    PrintMatrix<double>( Matrix, 1, 3, 1, 3 );
    printf("------------------------------------\n");
    PrintMatrix<double>( Matrix, 1, 2, 1, 2 );

    checkFloat000<double>( Matrix );

    writeCSVMatrix<double>( Matrix, std::string("out001.csv"), ',');
}

TEST (Util_IO, IntIn ) { 
    std::vector<std::vector<int> > Matrix;
    Matrix = readCSVMatrix<int>( std::string(my_argv[1]) + std::string("/int_in000.csv"), ',');
    // PrintMatrix<int>( Matrix );
    // PrintMatrix<int>( Matrix, 0, 2, 0, 2 );
    // printf("------------------------------------\n");
    // PrintMatrix<int>( Matrix, 1, 3, 1, 3 );
    // printf("------------------------------------\n");
    // PrintMatrix<int>( Matrix, 1, 2, 1, 2 );
    checkInt000( Matrix );
}

TEST (Util_IO, floatInOut ) { 
    std::vector<std::vector<double> > Matrix;
    // Matrix = readCSVMatrix<double>( std::string(my_argv[1]) + std::string( "/float_in000.csv"), ',');
    Matrix = readCSVMatrix<double>( std::string(my_argv[1]) + std::string( "/float_in000.csv"), ',');
    checkFloat000( Matrix );
    writeCSVMatrix<double>( Matrix, std::string("tmp.csv"), ',');
    std::vector<std::vector<double> > Matrix2;
    Matrix2 = readCSVMatrix<double>( std::string("tmp.csv"), ',');
    checkFloat000<double>( Matrix2 );
}

TEST (Util_IO,  IntInOut ) { 
    std::vector<std::vector<int> > Matrix;
    // Matrix = readCSVMatrix<int>( std::string("data/UnitCheckData/int_in000.csv"), ',');
    Matrix = readCSVMatrix<int>( std::string(my_argv[1]) + std::string("/int_in000.csv"), ',');
    checkInt000( Matrix );
    writeCSVMatrix<int>( Matrix, std::string("tmp.csv"), ',');
    std::vector<std::vector<int> > Matrix2;
    Matrix2 = readCSVMatrix<int>( std::string("tmp.csv"), ',');
    checkInt000( Matrix2 );
}

TEST (Util_IO,  ZeroInit ) { 
    std::vector<std::vector<float> > Matrix;
    Matrix = InitZeros<float>( 5 );
    EXPECT_EQ (5, Matrix.size() );
    EXPECT_EQ (5, Matrix[0].size() );
    EXPECT_EQ (5, Matrix[1].size() );
    EXPECT_EQ (5, Matrix[2].size() );
    EXPECT_EQ (5, Matrix[3].size() );
    EXPECT_EQ (5, Matrix[4].size() );

    for( int i = 0; i<5; ++i)
    {
        for( int j = 0; j<5; ++j )
        {
            EXPECT_FLOAT_EQ (0.0, Matrix[i][j]);
        }
    }
}

TEST (Util_IO,  ZeroInit2 ) { 
    std::vector<std::vector<float> > Matrix;
    Matrix = InitZeros<float>( 3, 7 );
    EXPECT_EQ (3, Matrix.size() );
    EXPECT_EQ (7, Matrix[0].size() );
    EXPECT_EQ (7, Matrix[1].size() );
    EXPECT_EQ (7, Matrix[2].size() );

    for( int i = 0; i<3; ++i)
    {
        for( int j = 0; j<7; ++j )
        {
            EXPECT_FLOAT_EQ (0.0, Matrix[i][j]);
        }
    }
}

// ########################################################################
// PAGE RANK HELP KERNELS
// ########################################################################
TEST (PageRank,  Dense2Sparse ) { 
    std::vector<std::vector<RefNumberType> > Matrix;
    // Matrix = readCSVMatrix<RefNumberType>( std::string("data/UnitCheckData/float_in000.csv"), ',');
    Matrix = readCSVMatrix<RefNumberType>( std::string(my_argv[1]) + std::string( "/float_in000.csv"), ',');
    PrintMatrix<RefNumberType>( Matrix );
    size_t n = Matrix.size();

    CSRType<RefNumberType> S = Dense2Sparse( Matrix );
    PrintCSR<RefNumberType>( S, n );
    std::vector<std::vector<RefNumberType> > Matrix2;
    Matrix2 = Sparse2Dense( S, n );

    PrintMatrix<RefNumberType>( Matrix2 );

    checkFloat000<RefNumberType>( Matrix2 );
}


int foo( int T )
{
    volatile int sum = 0;
    for(int t = 0; t<T; ++t)
    {
        volatile int sum1 = 0;
        for( int i = 0; i< 1e4; ++i )
        {
            sum1 += i*t;
        }
        sum += ( sum1 % t);
    }
    return sum;
}

std::vector<double> measure_foo(int T, int runs)
{
    std::vector<double> times;
    
    for( unsigned run = 0; run < runs; ++run )
    {
        
        std::clock_t now = std::clock();
        auto t_start = std::chrono::high_resolution_clock::now();

        foo(T);

        std::clock_t stop = std::clock();
        double time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;
        auto t_end = std::chrono::high_resolution_clock::now();

        // printf("%.2f ms (CLOCK)  %.2fms (WALL)\n", time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() );

        times.push_back( (double) std::chrono::duration<double, std::milli>(t_end-t_start).count() );
    }
    return times;
}

void showTimes( std::vector<double> &times ) 
{
    printf("%10.2f ms +/- %4.2f ms \t\t|", avg(times), stdev(times));
    for( size_t j = 0; j<times.size(); ++j)
    {
        printf("%8.2f\t", times[j]);
    }
    printf("\n");   
}


int calibrate(int targetMS )
{
    int T = 1e4;

    // calibrates a dummy block foo(T) with the parameter T such that foo(T) roughly 
    // takes targetMS ms to evaluate. We use a linear timing model. And perform two corrections. 

    std::vector<double> times = measure_foo( T, 3 );

    printf("T=%3i: \n", T);
    showTimes( times );

    T = (int)( 0.5 +  ((double) T) * (((double) targetMS) / avg(times) ) );

    int Told = 0;

    for( int i = 0; i<2; ++i )
    {
        Told = T;
        times = measure_foo( T, 3 );

        printf("T=%3i: \n", T);
        showTimes( times );
        T = (int)( 0.5 + ((double) T) * (((double) targetMS) / avg(times) ) );
    }

    return T;
}

TEST (timing,  claibrateFoo ) { 
    int T = calibrate( 100 ); // the evaluation of foo(T) takes now around the target value of time in micro seconds. 
    // foo(T);
}



std::vector<double> measure_openMPThreads(int T, int runs, int thr )
{
    std::vector<double> times;
    
    for( unsigned run = 0; run < runs; ++run )
    {
        
        std::clock_t now = std::clock();
        auto t_start = std::chrono::high_resolution_clock::now();
        // CRITICAL CODE
  
        int th_id, nthreads;
        #pragma omp parallel private(th_id) num_threads(thr)
        {
          th_id = omp_get_thread_num();
          foo(T);
        }

        // ENO OF CRITICAL CODE 
        std::clock_t stop = std::clock();
        double time_out = ((double)( stop - now )) / (double) CLOCKS_PER_SEC * 1000.0;
        auto t_end = std::chrono::high_resolution_clock::now();

        printf("%.2f ms (CLOCK)  %.2fms (WALL)\n", time_out, std::chrono::duration<double, std::milli>(t_end-t_start).count() );

        times.push_back( (double) std::chrono::duration<double, std::milli>(t_end-t_start).count() );
    }
    return times;
}


// ########################################################################
// CHECKS FOR MULTI-THREAD SUPPORT WITH OPENMP ( expects at least 2 threads)
// ########################################################################
TEST (openMP,  MultiThread ) { 
  int th_id, nthreads;
  #pragma omp parallel private(th_id)
  {
    th_id = omp_get_thread_num();
    printf("Hello World from thread %d\n", th_id);
    #pragma omp barrier
    if ( th_id == 0 ) {
      nthreads = omp_get_num_threads();
      printf("There are %d threads\n",nthreads);
    }
  }

  EXPECT_TRUE( nthreads > 1 );

}

TEST (openMP,  MultiThread_NumThreads ) { 
    #pragma omp parallel
    {
        int t;
        #pragma omp barrier
        for (t = 0; t < omp_get_max_threads(); ++t)
        {
            #pragma omp flush(t)
            if (t == omp_get_thread_num())
            {
                //printf("ProcName: %s; ProcID: %2d:%2ld; MyRank: %2d:%2d; MyThreadID %2d:%2d\n", name, sched_getcpu(), sysconf( _SC_NPROCESSORS_ONLN ), my_rank, size, omp_get_thread_num(), omp_get_max_threads() );
                // Supported on LINUX and POWER
                // printf("MyThreadID %2d:%2d runs on ProcID: %2d:%2ld\n",  omp_get_thread_num(), omp_get_max_threads(), sched_getcpu(), sysconf( _SC_NPROCESSORS_ONLN ) );
                // Restricted for MAC
                printf("MyThreadID %2d:%2d runs on ProcID: xxxx:%2ld\n",  omp_get_thread_num(), omp_get_max_threads(), sysconf( _SC_NPROCESSORS_ONLN ) );

            }
            #pragma omp flush(t)
            #pragma omp barrier
        }
    }
}

TEST (openMP,  GetGoodConfig ) { 
    int T = calibrate( 200 );
    int reps = 3;

    std::vector<int> thrs = {1,4,16,32,64,128,256,512,1024};

    for( unsigned i =0; i<thrs.size(); ++i)
    {
        int thr = thrs[i];
        std::vector<double> times = measure_openMPThreads( T, reps, thr );
        printf("Threads=%3i: \n", thr);
        showTimes( times );
    }
}


// ########################################################################
// CHECKS FOR GPU HELPER ROUTINES
// ########################################################################
TEST (PageRank,  FlattenData ) { 
    std::vector<std::vector<RefNumberType> > Matrix;
    Matrix = readCSVMatrix<RefNumberType>( std::string(my_argv[1]) + std::string( "/float_in000.csv"), ',');
    PrintMatrix<RefNumberType>( Matrix );
    size_t n = Matrix.size();

    RefNumberType* dataA = new RefNumberType[n*n];

    MatrixToFlatData( dataA, Matrix );

    std::vector<std::vector<RefNumberType> > Matrix2;
    Matrix2 = FlatDataToMatrix( dataA, n );

    delete[] dataA;

    PrintMatrix<RefNumberType>( Matrix2 );

    checkFloat000<RefNumberType>( Matrix2 );
}

// ########################################################################
// CHECKS FOR GPU SUPPORT
// ########################################################################
/*
TEST (CUDA_GPU,  BasicCode ) { 
    cudaDeviceProp  prop;

    int count;
    HANDLE_ERROR( cudaGetDeviceCount( &count ) );
    for (int i=0; i< count; i++) {
        HANDLE_ERROR( cudaGetDeviceProperties( &prop, i ) );
        printf( "   --- General Information for device %d ---\n", i );
        printf( "Name:  %s\n", prop.name );
        printf( "Compute capability:  %d.%d\n", prop.major, prop.minor );
        printf( "Clock rate:  %d\n", prop.clockRate );
        printf( "Device copy overlap:  " );
        if (prop.deviceOverlap)
            printf( "Enabled\n" );
        else
            printf( "Disabled\n");
        printf( "Kernel execution timeout :  " );
        if (prop.kernelExecTimeoutEnabled)
            printf( "Enabled\n" );
        else
            printf( "Disabled\n" );

        printf( "   --- Memory Information for device %d ---\n", i );
        printf( "Total global mem:  %ld\n", prop.totalGlobalMem );
        printf( "Total constant Mem:  %ld\n", prop.totalConstMem );
        printf( "Max mem pitch:  %ld\n", prop.memPitch );
        printf( "Texture Alignment:  %ld\n", prop.textureAlignment );

        printf( "   --- MP Information for device %d ---\n", i );
        printf( "Multiprocessor count:  %d\n",
                    prop.multiProcessorCount );
        printf( "Shared mem per mp:  %ld\n", prop.sharedMemPerBlock );
        printf( "Registers per mp:  %d\n", prop.regsPerBlock );
        printf( "Threads in warp:  %d\n", prop.warpSize );
        printf( "Max threads per block:  %d\n",
                    prop.maxThreadsPerBlock );
        printf( "Max thread dimensions:  (%d, %d, %d)\n",
                    prop.maxThreadsDim[0], prop.maxThreadsDim[1],
                    prop.maxThreadsDim[2] );
        printf( "Max grid dimensions:  (%d, %d, %d)\n",
                    prop.maxGridSize[0], prop.maxGridSize[1],
                    prop.maxGridSize[2] );
        printf( "\n" );
    }
} //*/

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    my_argc = argc;
    my_argv = argv;

    if( argc != 2 )
    {
        printf("Usage: ./MB000 <GTEST PARAMS> <DIRECTORY TO DATA>\n");
        exit(1);
    }

    return RUN_ALL_TESTS();
} 


