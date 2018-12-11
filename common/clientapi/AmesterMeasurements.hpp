#pragma once 
// #include "amester.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <ctime>
#include <chrono>

// The commands used in the socket communication.
// Protocal:
/*

		[HOST]		 				[DEVICE]								  [OCC (Measurement HW)]
		   | Start Server				|										|
		   | listen			 			|										|
		   |							|										|
		   |							|										|
		 ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
		   |							|										|
		   |							| Start CODE 							|
		   |							| 										|
		   |							|										|
		   |							|										|
		   |		CMD_START			| start()								|
		   | <------------------------- | wait_for_Ack() 						|
	***    |							| +++++++ 								|
	***    | -----------------------------------------------------------------> | configure.
	***	   |							| +++++++ 								|
	***    | -----------------------------------------------------------------> | start trace
	***	   |							| +++++++ 								|	*
	***	   |		CMD_ACK				| +++++++ 								|	*	
		   | -------------------------> | +++++++ return from start()			|	*
		   |							|										|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|	*** KERNEL CODE ***					|	*
		   |							|										|	*
		   |		CMD_STOP			| stop() 								| 	*
		   | <------------------------- | wait_for_Ack() 						|	*
	***	   |							| +++++++ 								|	*
	***    | -----------------------------------------------------------------> | stop trace
	***	   |							| +++++++ 								|
	***    | <----------------------------------------------------------------- | download data
	***    | <----------------------------------------------------------------- | download data
	***    | <----------------------------------------------------------------- | download data
	***    | <----------------------------------------------------------------- | download data
	***    | <----------------------------------------------------------------- | download data
	***	   |							| +++++++ 	 							|
	***	   |		CMD_ACK				| +++++++ 								|
		   | -------------------------> | +++++++ return from stop()			|
		   |							|										|
		   |							|										|
		   V 							V 										V

//*/

//enum State{INIT, RUNNING, STOPPED};
//
const static int BUFFER_SIZE  = 512;
const static int ADDR_SIZE    = 128;

const static char* CMD_CONFIG = "config:";
const static char* CMD_CONFIG_FILEBASE = "filebase:";

const static char* CMD_START = "start\n";
const static char* CMD_STOP  = "stop\n";
const static char* CMD_CLOSE = "close\n";

const static char* CMD_ACK   = "ack";
const int N_CMP = 3;


class Timer
{
	// class to measure time
	// uasege:
	// start() ###### stop() .... .... ... show() ... start() ########## stop() show() *2
	// measures time in #### and accumulates over multiple periodes. show() prints the results from the last stop() point.

	protected:
		std::clock_t _start_clock;
		std::chrono::high_resolution_clock::time_point _start_wall;

		double _clock;
		double _wall;

	public:
		Timer()
		{
			reset();
		}

		void reset()
		{
			_clock = 0;
			_wall  = 0;
		}

		void start()
		{
			_start_clock = std::clock();
			_start_wall  = std::chrono::high_resolution_clock::now();
		}

		void stop()
		{
			std::clock_t stop = std::clock();
			auto t_end = std::chrono::high_resolution_clock::now();

			double time_out = ((double)( stop - _start_clock )) / (double) CLOCKS_PER_SEC * 1000.0;
			_clock += time_out;
	
			_wall += std::chrono::duration<double, std::milli>(t_end - _start_wall).count();

		}

		double getClock(){ return _clock; } 
		double getWall(){ return _wall; }

		void show()
		{
			printf("CLOCK: %6.2f ms, WALL: %6.2f ms\n", getClock(), getWall() );
		}

		void show(double correction)
		{
			printf("[corr=%f , corr^-1= %f] CORR_CLOCK: %6.2f ms, CORR_WALL: %6.2f ms\n", correction, 1/correction, getClock()*correction, getWall()*correction );
		}
};


class AmesterMeasurements
{
	protected:
		int _sockfd;
		char _buffer[BUFFER_SIZE];
		char _addr[ADDR_SIZE];
		int _port;
		std::string _FileBaseStr;

		Timer _overhead_start;
		Timer _overhead_stop;
		Timer _kernel;
		Timer _full;

		int open_socket();
		void send_buffer(const char* const buffer);
		void wait_for_Ack();

	public:
		const static int NO_AMESTER   = -1;

		AmesterMeasurements();
		AmesterMeasurements(std::string AmesterConfigStr, std::string FileBaseStr, std::string addr, int port );

		~AmesterMeasurements();

		void start();
		void stop(double correction=1.0);
};

AmesterMeasurements::AmesterMeasurements()
{
	AmesterMeasurements("BASIC_PWR:ACCU", "Out", "scaligera", 9999 );
}

AmesterMeasurements::AmesterMeasurements(std::string AmesterConfigStr, std::string FileBaseStr, std::string addr, int port )
{
	strcpy(_addr, addr.c_str());    
	_port = port;
	_FileBaseStr = FileBaseStr;

	if( _port != NO_AMESTER )
	{
	    open_socket();

	    std::string msg = CMD_CONFIG + AmesterConfigStr + "\n";
	    send_buffer( msg.c_str() );
	    wait_for_Ack();

	    std::string msg2 = CMD_CONFIG_FILEBASE + FileBaseStr + "\n";
	    send_buffer( msg2.c_str() );
	    wait_for_Ack();
	}
}

AmesterMeasurements::~AmesterMeasurements()
{
	if( _port != NO_AMESTER )
	{
		send_buffer( CMD_CLOSE );
		close(_sockfd);
	}
}


void AmesterMeasurements::start()
{
	_full.start();
	_overhead_start.start();

	if( _port != NO_AMESTER )
	{
    	send_buffer( CMD_START );
    	wait_for_Ack();
	}

    _overhead_start.stop();
    _kernel.start();
}

void AmesterMeasurements::stop(double correction)
{
	_kernel.stop();
	_overhead_stop.start();

	if( _port != NO_AMESTER )
	{
		send_buffer( CMD_STOP );
		wait_for_Ack();
	}

	_overhead_stop.stop();
	_full.stop();

	// Show Info:
	#ifdef VERBOSE 
		printf("AmesterMeasurements:Start Overhead: \t"); _overhead_start.show();
		printf("AmesterMeasurements:Stop Overhead:  \t"); _overhead_stop.show();
		printf("AmesterMeasurements:KERNEL TIME:    \t"); _kernel.show();
		printf("AmesterMeasurements:FULL TIME (Measured): \t"); _full.show();
		printf("AmesterMeasurements:FULL TIME (Measured): \t"); _full.show(correction);
	#else
                printf("AmesterMeasurements:KERNEL TIME:    \t"); _kernel.show();
                printf("AmesterMeasurements:KERNEL TIME:    \t"); _kernel.show(correction);
	#endif 

	// Write Kernel Info to file:
	std::string file = _FileBaseStr + "_time.csv";
	printf("AmesterMeasurements:Writing File: %s ...", file.c_str());

	FILE* fp = fopen(file.c_str(), "w");
    if (!fp)
    {
        printf("AmesterMeasurements:File %s can not be created!\n", file.c_str());
	exit(1);
    }

    fprintf(fp, "%f, %f\n", _kernel.getClock()*correction, _kernel.getWall()*correction );
    fclose(fp);
    printf("[OK]\n");
}

void AmesterMeasurements::send_buffer(const char* const buffer)
{
    if (write(_sockfd, buffer, strlen(buffer)) < 0)
    {
        printf("AmesterMeasurements:Writing '%s' failed", buffer );
        exit(1);
    }
}

void AmesterMeasurements::wait_for_Ack()
{
    #ifdef VERBOSE 
	printf("AmesterMeasurements:wait_for_Ack: << wait >>\n");
    #endif
    bzero(_buffer, BUFFER_SIZE);
    int n = read(_sockfd, _buffer, BUFFER_SIZE-1);
    #ifdef VERBOSE 
	printf("AmesterMeasurements:wait_for_Ack: [OK]\n"); // got buffer content (n=%i): \"%s\"\n", n, _buffer );
    #endif
    if( strncmp( _buffer, CMD_ACK, N_CMP) != 0)
    {
    	printf("AmesterMeasurements:wait_for_Ack: Failed to obtain an ACK!\n");
    	if(n == 0) printf("AmesterMeasurements:Reason might be a closed socket on the server side (message length = 0)\n");
    	else printf("AmesterMeasurements:Reason, Wrong Answer: (n=%i): \"%s\"\n", n, _buffer );	
    	exit(1);
    }
}

int AmesterMeasurements::open_socket()
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        printf("AmesterMeasurements: Not able to open (new) socket\n");
        exit(1);
    }

    int flag = 1;
    int result = setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,  sizeof(int));
    if (result < 0)
    {
        printf("AmesterMeasurements: Not able to setup socket\n");
        exit(1);
    }
    struct hostent* server = gethostbyname(_addr);
    if (server == NULL)
    {
        printf("AmesterMeasurements: No such host: %s!\n", _addr );
        exit(1);
    }

    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(_port);

    if (connect(_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("AmesterMeasurements: Connection to %s:%i failed!\nIs there a server listening on the endpoint?\n", _addr, _port );
        exit(1);
    }

    return 1;
}
