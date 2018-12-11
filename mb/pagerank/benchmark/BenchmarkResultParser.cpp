#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <sstream>

#include "kernels.h"
#include "IO.hpp"
#include "Show.hpp"


struct DataFileType
{
	std::string file;
	unsigned dIdx;
	unsigned pIdx;
	unsigned rIdx;
};

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
// Function to analyze Measurements.
// Each Measurement has three files:
// 	*_header.csv (to describe the fields)
//  *_data.csv (that has two rows, for each sensor three values (timestamp, accu, update))
//  *_time.csv (two values: clock and wall time)
// INPUT: InFile
// Returns by reference, vector with name of headers and with values and a time vector.

void ReadAndParseMeasuredAmesterFile( std::vector<std::string> & headers, std::vector<double> & values, std::vector<double> & time, std::string InFile )
{
	headers 									= readCSVHeaderLine<std::string>( InFile+ "_header.csv", ',' );
	std::vector<std::vector<double> > rawData 	= readCSVMatrix<double>( InFile + "_data.csv", ',');
	time										= readCSVLine<double>( InFile + "_time.csv", ',');

	if( rawData.size() != 2 )
	{
		printf("ERROR: Measured Data file should contain TWO rows, it has %i rows\n",  (int) rawData.size() );
		exit(1);
	}

	if( rawData[0].size() != headers.size()*3 )
	{
		printf("ERROR: Measured Data should contain 3 times more columns than the header file (time, acc, update) values for each senosor. Headers: %i, DataCols: %i\n", (int) headers.size(), (int) rawData[0].size() );
		exit(1);
	}

	// PrintMatrix( rawData );

	values.clear();
	values.resize( headers.size() );

	for( unsigned i = 0; i<headers.size(); ++i )
	{
		// DATA CHUNCK FORMAT:
		// t1, acc1, up1
		// t2, acc2, up2

		// read row1
		double t1 	= rawData[0][3*i+0];
		double acc1 = rawData[0][3*i+1];
		double up1  = rawData[0][3*i+2];
		// read row2
		double t2 	= rawData[1][3*i+0];
		double acc2 = rawData[1][3*i+1];
		double up2  = rawData[1][3*i+2];

		double value = (acc2-acc1) / (up2 -up1);

		if( std::isnan( value ) )
		{
			printf("NAN WARNING: i=%i, SENSOR: %s\n", i, headers[i].c_str());
			printf("	acc1 = %e\n", acc1 );
			printf("	acc2 = %e\n", acc2 );
			printf("	up1  = %e\n", up1 );
			printf("	up2  = %e\n", up2 );
		}

		if( up2 < up1 )
		{
			printf("up2 < up1 (Overflow Error?) WARNING: i=%i, SENSOR: %s\n", i, headers[i].c_str());
			printf("	acc1 = %e\n", acc1 );
			printf("	acc2 = %e\n", acc2 );
			printf("	up1  = %e\n", up1 );
			printf("	up2  = %e\n", up2 );
			exit(1);
		}

		if( acc2 < acc1 )
		{
			printf("acc2 < acc1 (Overflow Error?) WARNING: i=%i, SENSOR: %s\n", i, headers[i].c_str());
			printf("	acc1 = %e\n", acc1 );
			printf("	acc2 = %e\n", acc2 );
			printf("	up1  = %e\n", up1 );
			printf("	up2  = %e\n", up2 );
			exit(1);
		}

		values[i] = value;
	}
}

void MergeResults( std::vector<DataFileType> DataFiles, std::vector<double> & mean_values, std::vector<double> & dev_values, std::vector<double> & mean_time, std::vector<double> & dev_time )
{
	int sensors = -1;

	// Outer vector, contains the sensor index.
	// Inner vector, contains the values.

	std::vector< std::vector<double> > fullData;
	std::vector< std::vector<double> > fullTime;

	for( unsigned i = 0; i<DataFiles.size(); ++i )
	{
		// Read for each file i:
		std::vector<std::string> headers;
		std::vector<double> values;
		std::vector<double> time;

		ReadAndParseMeasuredAmesterFile( headers, values, time, DataFiles[i].file );

		if( i==0 )
		{
			sensors = (int) headers.size();
			mean_values =	std::vector<double>(sensors, 0 );
			dev_values 	=	std::vector<double>(sensors, 0 );
			fullData.resize(sensors);
			fullTime.resize( time.size() );
		}else
		{
			if( sensors != headers.size() )
			{
				printf("MERGE RESULT ERROR:\n");
				printf("FILE %3i: (sensors=%3i) %s\n", 0, sensors, DataFiles[0].file.c_str() );
				printf("FILE %3i: (sensors=%3i) %s\n", i, (int) headers.size(), DataFiles[i].file.c_str() );
				exit(1);
			}
		}

		// GET FULL DATA.
		for( unsigned j = 0; j<sensors; ++j )
		{
			fullData[j].push_back( values[j] );
		}

		assert( time.size() == 2 ); // CLOCK AND WALL
		fullTime[0].push_back( time[0] );
		fullTime[1].push_back( time[1] );
	}

	// GET FULL DATA.
	for( unsigned j = 0; j<sensors; ++j )
	{
		mean_values[j] = avg( fullData[j] );
		dev_values[j]  = stdev( fullData[j] );
	}

	mean_time =	std::vector<double>(2, 0 );
	dev_time =	std::vector<double>(2, 0 );

	mean_time[0] = avg( fullTime[0] );
	mean_time[1] = avg( fullTime[1] );

	dev_time[0] = stdev( fullTime[0] );
	dev_time[1] = stdev( fullTime[1] );
}


void WriteCSVFile(  std::string OutFile,
					std::vector<unsigned> & all_dIdx,
					std::vector<unsigned> & all_pIdx,
					std::vector<unsigned> & all_rIdx,
					std::vector<std::string> & all_sensors,
					std::vector<double> & all_values )
{
	printf("Writing File: %s ... ", OutFile.c_str());
	FILE *fp = fopen( OutFile.c_str(), "w");
	if( fp == 0 )
	{
		printf("FileStream Error\n");
		exit(-1);
	}

	// WriteHeader
	fprintf(fp, "dIdx;pIdx;rIdx;Sensor;Value\n");

	for( size_t i=0; i<all_dIdx.size(); ++i )
	{
		//write row i
		fprintf(fp, "%3u;", all_dIdx[i]);
		fprintf(fp, "%3u;", all_pIdx[i]);
		fprintf(fp, "%3u;", all_rIdx[i]);
		fprintf(fp, "%s;", all_sensors[i].c_str() );
		fprintf(fp, "%.20f\n", all_values[i]);
	}

	fclose(fp);
	printf("(written %i lines) [OK]\n", (int) all_dIdx.size() );
}


void GenerateLongFormatCSV(	std::vector<DataFileType> DataFiles, std::string OutFile )
{
	int Nsensors = -1;

	std::vector<unsigned> all_dIdx;
	std::vector<unsigned> all_pIdx;
	std::vector<unsigned> all_rIdx;
	std::vector<std::string> all_sensors;
	std::vector<double> all_values;

	for( unsigned i = 0; i<DataFiles.size(); ++i )
	{
		// Read for each file i:
		std::vector<std::string> headers;
		std::vector<double> values;
		std::vector<double> time;

		ReadAndParseMeasuredAmesterFile( headers, values, time, DataFiles[i].file );

		if( i==0 )
		{
			Nsensors = (int) headers.size();
		}else
		{
			if( Nsensors != headers.size() )
			{
				printf("MERGE RESULT ERROR:\n");
				printf("FILE %3i: (sensors=%3i) %s\n", 0, Nsensors, DataFiles[0].file.c_str() );
				printf("FILE %3i: (sensors=%3i) %s\n", i, (int) headers.size(), DataFiles[i].file.c_str() );
				exit(1);
			}
		}

		// Data, Parameter and Rep settings.
		all_dIdx.insert( all_dIdx.end(), Nsensors+2, DataFiles[i].dIdx );
		all_pIdx.insert( all_pIdx.end(), Nsensors+2, DataFiles[i].pIdx );
		all_rIdx.insert( all_rIdx.end(), Nsensors+2, DataFiles[i].rIdx );

		assert( time.size() == 2 ); // CLOCK AND WALL
		all_sensors.push_back("TIME_CLOCK");
		all_values.push_back( time[0] );

		all_sensors.push_back("TIME_WALL");
		all_values.push_back( time[1] );

		// Sensor Values
		all_sensors.insert( all_sensors.end(), headers.begin(), headers.end());
		all_values.insert( all_values.end(), values.begin(), values.end() );

		WriteCSVFile( OutFile,
						all_dIdx,
						all_pIdx,
						all_rIdx,
						all_sensors,
						all_values );
	}
}

std::vector<DataFileType> GenerateFileSelection( std::string prefix,
												unsigned DataIdx_low, 	unsigned DataIdx_high,
												unsigned ParamIdx_low, 	unsigned ParamIdx_high,
												unsigned RepIdx_low, 	unsigned RepIdx_high )
{
	// assert( DataIdx_low 	< DataIdx_high );
	// assert( ParamIdx_low 	< ParamIdx_high );
	// assert( RepIdx_low 		< RepIdx_high );

	// assert( DataIdx_high 	<= _dataInput.size() );
	// assert( ParamIdx_high 	<= _modes.size() );
	// std::vector< std::string > files;
	std::vector<DataFileType> ret;


	for( unsigned rep = RepIdx_low; rep < RepIdx_high; ++rep ) // Rep Loop
	{
		for( unsigned dIdx = DataIdx_low; dIdx < DataIdx_high; ++dIdx ) // Data Loop
		{
			for( unsigned pIdx = ParamIdx_low; pIdx < ParamIdx_high; ++pIdx ) // Param Loop
			{
				//Run( dIdx, pIdx, rep );
				DataFileType entry;
				entry.file = prefix + std::to_string(dIdx) + "_" + std::to_string(pIdx) + "_" + std::to_string(rep);
				entry.dIdx = dIdx;
				entry.pIdx = pIdx;
				entry.rIdx = rep;

				ret.push_back( entry );
			}
		}
	}

	return ret;
}

void CheckFileExistence( std::vector<DataFileType> &DataFiles )
{
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	printf("CHECKING FILES:\n");
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");

	std::vector<std::string> appendix;
	appendix.push_back("_time.csv");
	appendix.push_back("_header.csv");
	appendix.push_back("_data.csv");

	for( unsigned fIndex = 0; fIndex<DataFiles.size(); ++fIndex )
	{
		for(unsigned j = 0; j < appendix.size(); ++j )
		{
			std::string FileName = DataFiles[fIndex].file + appendix[j];

			printf("File: %s ", FileName.c_str());

			std::ifstream input( FileName.c_str()  );
			if( input.good() )
			{
				printf("\t\t\t[OK]\n");

			}else
			{
				printf("\t\t\t[FileStream Error]\n");
			}
		}
	}
}

void usage()
{
	printf("Usage: either of\n");
	printf("BenchmarkResultParser -check <PREFIX IN>\n");
	printf("BenchmarkResultParser -check <PREFIX IN> <Min1> <Max1> <Min2> <Max2> <Min3> <Max3>\n");
	printf("BenchmarkResultParser -parse <PREFIX IN> <OUT FILE>\n");
	printf("BenchmarkResultParser -parse <PREFIX IN> <Min1> <Max1> <Min2> <Max2> <Min3> <Max3> <OUT FILE>\n");
	exit(-1);
}

int main(int argc, char **argv) {

	if( argc <= 1) usage();

	std::vector<DataFileType> DataFiles;

	if( argc == 9 || argc == 10 )
	{
		DataFiles = GenerateFileSelection( argv[2],  atoi( argv[3]), atoi( argv[4]),	 atoi( argv[5]), atoi( argv[6]), 	 atoi( argv[7]), atoi( argv[8]) );
	}else if( argc == 3 || argc == 4)
	{
		std::string fileName = std::string(argv[2]) + std::string("config.txt");
		printf("Reading File: %s ... ", fileName.c_str());
		FILE *fp = fopen( fileName.c_str(), "r");
		if( fp == 0 )
		{
			printf("FileStream Error\n");
			exit(-1);
		}
		int a,b,c,d,e,f;
		fscanf(fp, "%i %i %i %i %i %i\n", &a, &b, &c, &d, &e, &f );
		fclose(fp);
		printf("[OK]\n %i %i %i %i %i %i\n", a, b, c, d, e, f );
		DataFiles = GenerateFileSelection( argv[2],  a, b, c, d, e, f );
	}else
	{
		usage();
	}

	if( std::string( argv[1] ) == "-check" )
	{
		CheckFileExistence(DataFiles);
	}else if( std::string( argv[1] ) == "-parse" )
	{
		if( argc == 4 )
		{
			GenerateLongFormatCSV( DataFiles, argv[3]);
		}else if( argc == 10 )
		{
			GenerateLongFormatCSV( DataFiles, argv[9]);
		}else
		{
			usage();
		}

	}else
	{
		printf("command: %s not known.\n", argv[1]);
		usage();
	}

	/*
	std::vector<std::string> headers;
	std::vector<double> values;
	std::vector<double> time;

	// int DataIdx 	= 0;
	// int ParamIdx 	= 1;
	// int RepIdx	 	= 0;

	// std::string InFile = "/fl/eid/DATA/Out001/Out_" + std::to_string(DataIdx) + "_" + std::to_string(ParamIdx) + "_" + std::to_string(RepIdx);

	//std::vector<std::string> files = GenerateFileSelection( "/fl/eid/DATA/Out001/Out_", 0,1,	0,1, 	0,3 );
	std::vector<DataFileType> DataFiles = GenerateFileSelection( "/fl/eid/DATA/Out001/Out_", 0,1,	0,1, 	0,3 );

	for( unsigned fIndex = 0; fIndex<DataFiles.size(); ++fIndex )
	{
		printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
		printf("READING FILE: %s\n", DataFiles[fIndex].file.c_str() );
		printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");

		ReadAndParseMeasuredAmesterFile( headers, values, time, DataFiles[fIndex].file);

		printf("     \tTIME: (%.3fms, %.3fms)\n", time[0], time[1] );

		for( unsigned i =0; i<headers.size(); ++i )
		{
			printf("i=%3i \tSENSOR: %40s: %.3f \n", i, headers[i].c_str(), values[i] );
		}
	}

	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");

	std::vector<double> mean_values;
	std::vector<double> dev_values;

	std::vector<double> mean_time;
	std::vector<double> dev_time;

	MergeResults( DataFiles, mean_values, dev_values, mean_time, dev_time );

	printf("    \tTIME_CLOCK: %12.3f +/- %10.3fms\n",  mean_time[0], dev_time[0] );
	printf("    \tTIME_WALL:  %12.3f +/- %10.3fms\n",  mean_time[1], dev_time[1] );

	for( unsigned i =0; i<headers.size(); ++i )
	{
			printf("i=%3i \tSENSOR: %40s: %12.3f +/- %10.3f \n", i, headers[i].c_str(), mean_values[i], dev_values[i] );
	}

	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");


	std::vector< std::vector<double> > all_mean_values;
	std::vector< std::vector<double> > all_dev_values;

	std::vector< std::vector<double> > all_mean_times;
	std::vector< std::vector<double> > all_dev_times;


	const int NumParams = 21;
	all_mean_values.resize( NumParams );
	all_dev_values.resize( NumParams );
	all_mean_times.resize( NumParams );
	all_dev_times.resize( NumParams );

	for( unsigned pIdx = 0; pIdx < NumParams; ++pIdx )
	{
		DataFiles = GenerateFileSelection( "/fl/eid/DATA/Out001/Out_", 0,1,	pIdx,pIdx+1, 	0,3 );
		MergeResults( DataFiles, all_mean_values[pIdx], all_dev_values[pIdx], all_mean_times[pIdx], all_dev_times[pIdx] );
	}

	printf("   \tTIME_CLOCK: %40s\t", " ");
		for( unsigned pIdx = 0; pIdx < NumParams; ++pIdx )
		{
			printf("%12.3f +/-%10.3f \t", all_mean_times[pIdx][0], all_dev_times[pIdx][0] );
		}
	printf("\n");

	printf("   \tTIME_WALL:  %40s\t", " ");
		for( unsigned pIdx = 0; pIdx < NumParams; ++pIdx )
		{
			printf("%12.3f +/-%10.3f \t", all_mean_times[pIdx][1], all_dev_times[pIdx][1] );
		}
	printf("\n");

	for( unsigned i =0; i<headers.size(); ++i )
	{
		printf("i=%3i, SENSOR: %40s:\t", i, headers[i].c_str() );
		for( unsigned pIdx = 0; pIdx < NumParams; ++pIdx )
		{
			printf("%12.3f +/-%10.3f \t", all_mean_values[pIdx][i], all_dev_values[pIdx][i] );
		}
		printf("\n");
	} */

	// printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	// printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");
	// printf("---------------------------------------------------------------------------------------------------------------------------------------------\n");

	// #DataFiles = GenerateFileSelection( "/fl/eid/DATA/Out001/Out_", 0,1,	0, 21, 	0,3 );
	// #GenerateLongFormatCSV( DataFiles, "/fl/eid/DATA/Out001/LongCSV.csv");

	printf("BENCHMARK RESULT PARSER ALL DONE\n");

}
