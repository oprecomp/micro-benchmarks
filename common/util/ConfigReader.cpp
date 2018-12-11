//////////////////////////////////////////////////////////
// Revisions  :
// Date        Author               Description
//             Florian Scheidegger  Created
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

#include "ConfigReader.h"
#include <iostream>
#include <fstream>
#include <sstream>

double ConfigReader::getValue( std::string variable )
{
	std::string buffer = getString( variable );
	double ret = 0;
	sscanf (buffer.c_str(),"%lf", &ret);
	return ret;
}

ConfigReader::ConfigReader(std::string fileName)
{
	_fileName = fileName;

	std::ifstream myfile(fileName.c_str());
	if(!myfile.is_open())
	{
	  std::cout << "Error: file " << fileName << " does not exist!" << std::endl;
	  exit(1);
	}
	std::cout << "File opend: " << fileName << std::endl;

	std::string tmpLine;
	int LineCnt = 0;

	char Buffer[256];
	char Buffer2[256];

	while(getline(myfile, tmpLine))
	{
		sscanf (tmpLine.c_str(),"%s %[^\n]",Buffer, Buffer2);

		_map[ std::string(Buffer) ] = Buffer2;
		_keys.push_back(std::string(Buffer));

  		LineCnt++;
	}

	myfile.close();
	std::cout << "Read " << LineCnt << " line(s). File closed: " << fileName << std::endl;
}

double ConfigReader::getDouble(std::string variable)
{
	return (double) getValue( variable );
}

float ConfigReader::getFloat(std::string variable)
{
	return (float) getValue( variable );

}

int ConfigReader::getInt(std::string variable)
{
	return (int) getValue( variable );
}

std::string ConfigReader::getString(std::string variable)
{
	std::map<std::string,std::string>::iterator it = _map.find(variable);
	if(it != _map.end())
	{
	   return it->second;
	}

	std::cout << "ConfigReader::Error: Variable does not exsist!\n";
	std::cout << " -> File: " << _fileName << std::endl;
	std::cout << " -> Variable: " << variable << std::endl;	
	exit(1);
}

std::string ConfigReader::getCitedString(std::string variable)
{
	std::string str = getString( variable );
	std::size_t pos1;
	std::size_t pos2;

	pos1=str.find("\"");
	if (pos1==std::string::npos) return str;

	pos2=str.find("\"", pos1+1 );

	if (pos2==std::string::npos)
	{
		std::cout << "There are not 2 \" in the string for variable " << variable << std::endl;
		exit(1);
	}

	return str.substr( pos1+1, pos2-pos1-1 );
}

template< typename T>
T ConfigReader::get(std::string variable)
{
	return static_cast<T>( getValue(variable) );
}

long long unsigned ConfigReader::getHash()
{
	long long unsigned ret = 0;

	int cnt = 0;
	 
	for(std::map<std::string, std::string>::iterator it = _map.begin(); it != _map.end(); ++it )
	{
		//ret = ret + it->second*cnt++;
		std::string str = it->second;

		long long unsigned r = 0;
		for (int i=0; i<str.length(); ++i)
		{
		    r += ((int) str[i])*i;
		}

		ret += (r*cnt++ % 1973523 );
	}

	return ret;
}

void ConfigReader::show()
{
	std::cout << "-----------------------------------------------------\n";
	std::cout << " CONFIGURATAION (" << _map.size() << "),  ";
	printf("hash: %llx\n", (long long unsigned)getHash() );
	std::cout << "-----------------------------------------------------\n";

	for( unsigned int i = 0; i<_keys.size(); ++i )
	{
		printf("%20s: %s\n", _keys[i].c_str(), _map[_keys[i]].c_str() );
	}

	std::cout << "-----------------------------------------------------\n\n";
}




