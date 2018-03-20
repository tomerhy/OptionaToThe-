
#pragma once

class op_manager : public threaded, public informer_cb_api
{
    pthread_mutex_t m_record_lock, m_event_lock;
    bool m_processed;
    trade_info_t m_record;
	pthread_cond_t m_event;

	informer * m_informer;

	double m_balance, m_pnl;

	void run();
	void process_record_update();
	void process_record(const trade_info_t &);
	static int valid_record(const trade_info_t &);
	static const strike_info_t * get_work_strike(const trade_info_t &);

public:
	op_manager();
	~op_manager();

	int init(const std::string & log_cat, const manager_conf * conf);
	int start();
	int stop();

	void trade_info_update(const trade_info_t &);
};
