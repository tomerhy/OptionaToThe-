
#pragma once

typedef struct
{
	int id;
}trade_request_t;

typedef struct
{
	int id, result;
}trade_result_t;

class executor_cb_api
{
public:
	virtual void trade_result(const trade_result_t &) = 0;
};

class executor : public threaded
{
private:
	executor_cb_api * m_manager;
protected:
	void report(const trade_result_t & result) { m_manager->trade_result(result); }
public:
	executor() : m_manager(NULL) {}
	virtual ~executor() {}

	void set_manager(executor_cb_api * manager) { m_manager = manager; }

	virtual int init(const std::string & log_cat, const executor_conf * conf) = 0;

	virtual void execute(const trade_request_t &) = 0;
};

