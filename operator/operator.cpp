
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

void get_options(int argc, char *argv[], std::string & conf_file);
void show_usage(const char * prog);

int main(int argc, char *argv[])
{
	std::string conf_file;
	get_options(argc, argv, conf_file);

	//config
	//log
	//objects
}

void get_options(int argc, char *argv[], std::string & conf_file)
{
	if(argc == 1)
	{
		show_usage(argv[0]);
		exit(0);
	}
	int opt;
	while ((opt = getopt(argc, argv, "hf:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			show_usage(argv[0]);
			exit(0);
		case 'f':
			conf_file = optarg;
			break;
		default:
			std::cerr << "Invalid program arguments." << std::endl;
			show_usage(argv[0]);
			exit(__LINE__);
		}
	}
}

void show_usage(const char * prog)
{
	std::cout << "Usage:" << std::endl;
	std::cout << prog << "   [ OPTIONS ]" << std::endl;
	std::cout << "-f   program configuration file" << std::endl;
}
