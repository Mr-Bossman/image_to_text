#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>

uint64_t Tmicro(){
	struct timeval time_now{};
	gettimeofday(&time_now, nullptr);
	return time_now.tv_usec;
}

int main(int argc, char **argv)
{
	std::ifstream Dfile(argv[1]);
	int fps = strtol(argv[2], NULL, 10);
	std::string line;
	uint64_t last = Tmicro();
	while (std::getline(Dfile, line))
	{
		std::cout << line << std::endl;
		if(line.substr(line.length()-3,line.length()-1)  == "[0m"){
			usleep((1000000ull/fps) - (Tmicro()-last));
			last = Tmicro();
		}
	}
	return 0;
}