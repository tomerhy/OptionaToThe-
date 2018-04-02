
#pragma once

class op_manager : public threaded, public informer_cb_api, public executor_cb_api
{
    pthread_mutex_t m_record_lock, m_event_lock;
    bool m_processed;
    trade_info_t m_record;
	pthread_cond_t m_event;

	informer * m_informer;
	executor * m_executor;

	double m_balance, m_pnl;
	bool m_in_position;

	void run();
	void process_record_update();
	void process_record(const trade_info_t &);
	void seek_out_of_trade(const trade_info_t &);
	void seek_into_trade(const trade_info_t &);

	static int valid_record(const trade_info_t &);
	static const strike_info_t * get_work_strike(const trade_info_t &);
	static u_int64_t congr_c_mod_m(const u_int64_t c, const u_int64_t m, const u_int64_t x);

public:
	op_manager();
	~op_manager();

	int init(const std::string & log_cat, const manager_conf * conf);
	int start();
	int stop();

	void trade_info_update(const trade_info_t &);
	void trade_result(const trade_result_t &);
};
