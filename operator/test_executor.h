
#pragma once

class test_executor : public executor
{
	u_int64_t m_total_requests, m_success_requests;
	pthread_mutex_t m_reque_lock, m_event_lock;
	pthread_cond_t m_event;
	std::deque<trade_request> m_reque;

	bool process_trade_requests();
	void process_trade_request(const trade_request &);
protected:
	void run();
public:
	test_executor();
	virtual ~test_executor();

	int init(const std::string & log_cat, const executor_conf * conf);

	void execute(const trade_request &);
};
