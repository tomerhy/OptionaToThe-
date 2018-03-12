
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>

#include <event2/event.h>

#include "config.h"
#include "op_manager.h"

void get_options(int argc, char *argv[], std::string & conf_file);
void show_usage(const char * prog);
int init_log(const logger_global_conf & log_conf);
void wait4ctrl_c();

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
    log4cpp::Category::getInstance(conf.log_conf->category).notice("operator log start");

	op_manager * mgr = new op_manager;

	if(0 != mgr->init(conf.log_conf->category, *conf.mngr_conf))
	{
		log4cpp::Category::getInstance(conf.log_conf->category).fatal("%s: failed to initialize the manager.");
		exit(__LINE__);
	}

	if(0 != mgr->start())
	{
		log4cpp::Category::getInstance(conf.log_conf->category).fatal("%s: failed to start the manager.");
		exit(__LINE__);
	}

	wait4ctrl_c();
	mgr->stop();
	delete mgr;
    log4cpp::Category::getInstance(conf.log_conf->category).notice("operator log stop");
	return 0;
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
    return 0;
}

void ctrl_c_cb(evutil_socket_t, short, void * arg)
{
    event_base_loopbreak((struct event_base *)arg);
}

void wait4ctrl_c()
{
	struct event_base * the_base = event_base_new();
	struct event * ctrl_c_evt = evsignal_new(the_base, 2/*=SIGINT*/, ctrl_c_cb, the_base);
	event_add(ctrl_c_evt, NULL);
	event_base_dispatch(the_base);
	event_free(ctrl_c_evt);
	event_base_free(the_base);
}
