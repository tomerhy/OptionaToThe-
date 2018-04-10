
#pragma once

class op_manager : public threaded, public informer_cb_api, public executor_cb_api
{
	typedef enum { mark = 0, set = 1, go = 2 } trade_state_t;

    pthread_mutex_t m_record_lock, m_event_lock;
    std::deque<record_base *> m_record_queue;
	pthread_cond_t m_event;

	informer * m_informer;
	executor * m_executor;

	double m_balance, m_pnl;
	trade_state_t m_position;
	trade_result m_going_trade;

	void run();
	bool process_record();
	void process_record(const record_base *);
	void process_trade_info_record(const trade_info & tirec);
	void process_trade_result_record(const trade_result & trrec);

	void seek_out_of_trade(const trade_info &);
	void seek_into_trade(const trade_info &);

	static int valid_record(const trade_info &);
	static const strike_info * get_work_strike(const trade_info &);
	static u_int64_t congr_c_mod_m(const u_int64_t c, const u_int64_t m, const u_int64_t x);

public:
	op_manager();
	~op_manager();

	int init(const std::string & log_cat, const manager_conf * conf);
	int start();
	int stop();

	void trade_info_update(const trade_info &);
	void on_trade_result(const trade_result &);
};
