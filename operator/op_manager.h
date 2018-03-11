
#pragma once

class manager_conf;

class op_manager
{
public:
	op_manager();
	~op_manager();

	int init(const manager_conf & conf);
	int launch();
};
