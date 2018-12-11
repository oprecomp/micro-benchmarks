#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "ConfigReader.h"

int main(int argc, char** args)
{
	
	ConfigReader Conf("test.config");

	Conf.show();


	std::cout << "TEST is " << Conf.getDouble("TEST") << std::endl;
	std::cout << "Foo is " << Conf.getDouble("Foo") << std::endl;
	std::cout << "k is " << Conf.getDouble("k") << std::endl;
	std::cout << "PATH is [" << Conf.getString("PATH") << "]" << std::endl;
	std::cout << "PATH2 is [" << Conf.getCitedString("PATH2") << "]" << std::endl;
	std::cout << "PATH3 is [" << Conf.getCitedString("PATH3") << "]" << std::endl;

	std::cout << "DONE\n";
	
	return 0;
}