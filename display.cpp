#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <bits/stdc++.h>

uint64_t Tmicro()
{
	struct timeval time_now{};
	gettimeofday(&time_now, nullptr);
	return time_now.tv_sec*1000000 + time_now.tv_usec;
}

std::string getframe(std::ifstream &Dfile)
{
	std::string line, frame;
	while (std::getline(Dfile, line))
	{
		frame += line + "\n";
		if(line.substr(line.length()-3,line.length()-1)  == "[0m")
			break;
	}
	return frame;
}
int main(int argc, char **argv)
{
	std::ifstream Dfile(argv[1]);
	int32_t Ftime = 1000000ull/strtol(argv[2], NULL, 10);
	uint64_t start = Tmicro();
	int32_t frame = 0;
	while(true){
		uint64_t delay = (Ftime*++frame)- (Tmicro()-start);
		auto framedis = getframe(Dfile);
		if(delay > 0){
			std::cout << framedis;
			usleep(delay);
		}
		if(framedis.length() < 1)
			break;
	}
	return 0;
}
