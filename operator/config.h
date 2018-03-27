
#pragma once

class logger_base_conf
{
public:
	logger_base_conf()
	: category("op"), level(3)
	{}
	virtual ~logger_base_conf() {}

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
	virtual ~logger_global_conf() {}

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
	virtual ~informer_conf()
	{
		delete log_conf;
	}

	logger_base_conf * log_conf;
};

class dreamer_conf : public informer_conf
{
public:
	dreamer_conf()
	: informer_conf(), srvc_port(0), local_port(0)
	{}
	~dreamer_conf()
	{}

	std::string srvc_addr;
	u_int16_t srvc_port;
	std::string local_addr;
	u_int16_t local_port;
};

class executor_conf
{
public:
	executor_conf()
	: log_conf(NULL)
	{}
	~executor_conf()
	{
		delete log_conf;
	}

	logger_base_conf * log_conf;
};

class test_executor_conf : public executor_conf
{
public:
	test_executor_conf()
	: executor_conf()
	{}
	virtual ~test_executor_conf()
	{
	}
};

class manager_conf
{
public:
	manager_conf()
	: log_conf(NULL), info_conf(NULL), exec_conf(NULL), informer_type(manager_conf::dreamer)
	, executor_type(manager_conf::test), balance(0), pnl(0)
	{}
	~manager_conf()
	{
		delete log_conf;
		delete info_conf;
		delete exec_conf;
	}

	enum { dreamer = 0 } informer_type;
	enum { test = 0 } executor_type;

	logger_base_conf * log_conf;
	informer_conf * info_conf;
	executor_conf * exec_conf;
	double balance, pnl;
};

class operator_conf
{
public:
	operator_conf()
	: log_conf(NULL), mngr_conf(NULL)
	{}
	~operator_conf()
	{
		delete log_conf;
		delete mngr_conf;
	}

	logger_global_conf * log_conf;
	manager_conf * mngr_conf;
};

int load_config(const char * config_file, operator_conf & conf);
