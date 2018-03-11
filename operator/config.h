
#pragma once

class logger_base_conf
{
public:
	logger_base_conf()
	: category("op"), level(3)
	{}

	std::string category;
	int level;
};

static const char default_layout[] = "%d{%y-%m-%d %H:%M:%S.%l}| %-6p | %-15c | %m%n";

class logger_global_conf : public logger_base_conf
{
public:
	logger_global_conf()
	: logger_base_conf()
	, layout(default_layout)
	, file("operator.log")
	, directory("./")
	, max_files(2)
	, max_size(5*1024*1024)
	{}

	std::string layout;
	std::string file;
	std::string directory;
	size_t max_files;
	size_t max_size;
};

class informer_conf
{
public:
	informer_conf()
	: log_conf(NULL)
	{}

	logger_base_conf * log_conf;
};

class executor_conf
{
public:
	executor_conf()
	: log_conf(NULL)
	{}

	logger_base_conf * log_conf;
};

class manager_conf
{
public:
	manager_conf()
	: log_conf(NULL), info_conf(NULL), exec_conf(NULL)
	{}

	logger_base_conf * log_conf;
	informer_conf * info_conf;
	executor_conf * exec_conf;
};

class operator_conf
{
public:
	operator_conf()
	: log_conf(NULL), mngr_conf(NULL)
	{}

	logger_global_conf * log_conf;
	manager_conf * mngr_conf;
};

int load_config(const char * config_file, operator_conf & conf);
