
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>

#include "config.h"

void get_options(int argc, char *argv[], std::string & conf_file);
void show_usage(const char * prog);
int init_log(const logger_global_conf & log_conf);

int main(int argc, char *argv[])
{
	std::string conf_file;
	get_options(argc, argv, conf_file);

	operator_conf conf;
	if(0 != load_config(conf_file.c_str(), conf))
	{
		std::cerr << "main: failed loading configuration file [" << conf_file << "]" << std::endl;
		exit(__LINE__);
	}

	if(0 != init_log(*conf.log_conf))
	{
		std::cerr << "main: failed to initialize the log" << std::endl;
		exit(__LINE__);
	}

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

int init_log(const logger_global_conf & log_conf)
{
	std::string log_file = log_conf.file;
	log_file.insert(0, "/");
	log_file.insert(0, log_conf.directory);

    log4cpp::Layout * log_layout = NULL;
    log4cpp::Appender * appender = new log4cpp::RollingFileAppender("lpm.appender", log_file.c_str(), log_conf.max_size, log_conf.max_files);

    bool pattern_layout = false;
    try
    {
        log_layout = new log4cpp::PatternLayout();
        ((log4cpp::PatternLayout *)log_layout)->setConversionPattern(log_conf.layout);
        appender->setLayout(log_layout);
        pattern_layout = true;
    }
    catch(...)
    {
        pattern_layout = false;
    }

    if(!pattern_layout)
    {
        log_layout = new log4cpp::BasicLayout();
        appender->setLayout(log_layout);
    }

    log4cpp::Category::getInstance(log_conf.category).addAppender(appender);
    log4cpp::Category::getInstance(log_conf.category).setPriority((log4cpp::Priority::PriorityLevel)log_conf.level);
    log4cpp::Category::getInstance(log_conf.category).notice("dreamon log start");
}
