//////////////////////////////////////////////////////////
// Revisions  :
// Date        Author               Description
//             Florian Scheidegger  Created
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
// g++ -Wall TestConfig.cpp ConfigReader.cpp -o TestConfig.x


#pragma once 

#include <stdio.h>
#include <stdlib.h> 
#include <map>
#include <vector>
#include <string>
#include <stdint.h>


// Class to handle parameters read from a file.
// Note, that parameters are accessed by a string identifier through a map data structure.

class ConfigReader
{
	protected:
		std::vector<std::string> _keys;
		std::map< std::string, std::string> _map;
		std::string _fileName;

		double getValue( std::string variable );

	public:

		// Initalization requires the configuration file
		ConfigReader(std::string fileName);

		// access methodes (for double, float or int)
		// (the input read after the variable must be castable into the wanted output format)
		double getDouble(std::string variable);
		float getFloat(std::string variable);
		int getInt(std::string variable);
		std::string getString(std::string variable);
		std::string getCitedString(std::string variable);

		// generic access
		template< typename T>
		T get(std::string variable);

		long long unsigned getHash();
		void show();
};