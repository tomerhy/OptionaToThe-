
#pragma once

#include <pthread.h>
#include <semaphore.h>

#include "record.h"
#include "informer.h"

class manager_conf;

class op_manager : public infomer_cb_api
{
    pthread_t m_proc_thread;
    std::string m_log_cat;

    pthread_mutex_t m_record_lock, m_event_lock;
    bool m_processed;
    trade_info_t m_record;
	pthread_cond_t m_event;

    friend void * manager_proc(void *);
	void run();
	void process_record_update();
	void process_record(const trade_info_t &);

    sem_t m_run_flag;
public:
	op_manager();
	~op_manager();

	int init(const std::string & log_cat, const manager_conf & conf);
	int start();
	int stop();

	void trade_info_update(const trade_info_t &);
};
