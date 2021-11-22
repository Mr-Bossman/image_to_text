#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
int main(int argc, char **argv)
{
	std::ifstream Dfile(argv[1]);
	int fps = strtol(argv[2], NULL, 10);
	std::string line;
	while (std::getline(Dfile, line))
	{
		std::cout << line << std::endl;
		if(line.substr(line.length()-3,line.length()-1)  == "[0m")
		usleep(1000000ull/fps);
	}
	return 0;
}