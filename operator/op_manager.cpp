
#include <stdlib.h>
#include <semaphore.h>

#include <string>
#include <deque>

#include <log4cpp/Category.hh>

#include "config.h"
#include "record.h"
#include "threaded.h"
#include "informer.h"
#include "executor.h"
#include "op_manager.h"
#include "dreamer.h"
#include "test_executor.h"

op_manager::op_manager()
: m_processed(true), m_informer(NULL), m_executor(NULL)
, m_balance(0.0), m_pnl(0.0), m_in_position(false)
{
	pthread_mutex_init(&m_record_lock, NULL);
	pthread_mutex_init(&m_event_lock, NULL);
	pthread_cond_init(&m_event, NULL);
}

op_manager::~op_manager()
{
	if(NULL != m_informer)
	{
		delete m_informer;
		m_informer = NULL;
	}

	if(NULL != m_executor)
	{
		delete m_executor;
		m_executor = NULL;
	}

	pthread_mutex_destroy(&m_record_lock);
	pthread_mutex_destroy(&m_event_lock);
	pthread_cond_destroy(&m_event);
}

int op_manager::init(const std::string & log_cat, const manager_conf * conf)
{
	m_log_cat = log_cat + '.' + conf->log_conf->category;
	log4cpp::Category::getInstance(m_log_cat).setPriority((log4cpp::Priority::PriorityLevel)conf->log_conf->level);

	switch(conf->informer_type)
	{
	case manager_conf::dreamer:
		log4cpp::Category::getInstance(m_log_cat).debug("%s: dreamer type informer set.", __FUNCTION__);
		m_informer = new dreamer;
		break;
	default:
		log4cpp::Category::getInstance(m_log_cat).error("%s: set dreamer type %u is not supported.", __FUNCTION__, (u_int32_t)conf->informer_type);
		return -1;
	}

	if(0 != m_informer->init(m_log_cat, conf->info_conf))
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: informer init() failed.", __FUNCTION__);
		return -1;
	}
	m_informer->set_manager(this);

	switch(conf->executor_type)
	{
	case manager_conf::test:
		log4cpp::Category::getInstance(m_log_cat).debug("%s: test type executor set.", __FUNCTION__);
		this->m_executor = new test_executor;
		break;
	default:
		log4cpp::Category::getInstance(m_log_cat).error("%s: set executor type %u is not supported.", __FUNCTION__, (u_int32_t)conf->informer_type);
		return -1;
	}

	if(0 != m_executor->init(m_log_cat, conf->exec_conf))
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: executor init() failed.", __FUNCTION__);
		return -1;
	}
	m_executor->set_manager(this);

	this->m_balance = conf->balance;
	this->m_pnl = conf->pnl;
	log4cpp::Category::getInstance(m_log_cat).notice("%s: balance=%f; P&L=%f;",	__FUNCTION__, m_balance, m_pnl);
	log4cpp::Category::getInstance(m_log_cat).info("%s: op_manager initialized.", __FUNCTION__);
	return 0;
}

int op_manager::start()
{
	if(0 == threaded::start())
	{
		if(0 == m_informer->start())
		{
			log4cpp::Category::getInstance(m_log_cat).info("%s: op_manager started.", __FUNCTION__);
			return 0;
		}
		else
			log4cpp::Category::getInstance(m_log_cat).error("%s: informer start() failed.", __FUNCTION__);
		threaded::stop();
	}
	else
		log4cpp::Category::getInstance(m_log_cat).error("%s: thread start() failed.", __FUNCTION__);
	return -1;
}

int op_manager::stop()
{
	int result = 0;
	if(0 != m_informer->stop())
	{
    	log4cpp::Category::getInstance(m_log_cat).error("%s: informer stop() failed.", __FUNCTION__);
    	result = -1;
	}
	if(0 != threaded::stop())
	{
    	log4cpp::Category::getInstance(m_log_cat).error("%s: thread stop() failed.", __FUNCTION__);
    	result = -1;
	}
	return result;
}

void op_manager::run()
{
	static const unsigned int idle_wait_ms = 250;

	struct timespec event_timeout;

	do
	{
		process_record_update();

		pthread_mutex_lock(&m_event_lock);
		clock_gettime(CLOCK_REALTIME, &event_timeout);
		event_timeout.tv_nsec += (idle_wait_ms * 1000000/*mili-to-nano*/);
		event_timeout.tv_sec += (event_timeout.tv_nsec / 1000000000);
		event_timeout.tv_nsec = (event_timeout.tv_nsec%1000000000);
		pthread_cond_timedwait(&m_event, &m_event_lock, &event_timeout);
		pthread_mutex_unlock(&m_event_lock);
	}while(0 == still_running());
}

void op_manager::trade_info_update(const trade_info_t & update)
{
	pthread_mutex_lock(&m_record_lock);
	m_record = update;
	m_processed = false;
	pthread_mutex_unlock(&m_record_lock);
	pthread_cond_signal(&m_event);
}

void op_manager::process_record_update()
{
	bool update = false;
	trade_info_t updated_record;

	pthread_mutex_lock(&m_record_lock);
	if(!m_processed)
	{
		m_processed = update = true;
		updated_record = m_record;
	}
	pthread_mutex_unlock(&m_record_lock);

	if(update)
		process_record(updated_record);
}

void op_manager::process_record(const trade_info_t & rec)
{
	log4cpp::Category::getInstance(m_log_cat).info("%s: processing new record update.", __FUNCTION__);
	log4cpp::Category::getInstance(m_log_cat).debug(trade_info_txt(rec));

	if(0 != valid_record(rec))
	{
		log4cpp::Category::getInstance(m_log_cat).warn("%s: invalid record dropped.", __FUNCTION__);
		return;
	}

	if (m_in_position)
		seek_out_of_trade(rec);
	else
		seek_into_trade(rec);
}

int op_manager::valid_record(const trade_info_t & ti)
{
	//this loop checks that all strikes' call & put
	//prices are divisible by 10 without remainder
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
		if((0 != ti.strikes[i].call.current%10) || (0 != ti.strikes[i].put.current%10))
			return -1;

	return 0;
}

void op_manager::seek_out_of_trade(const trade_info_t &)
{
	return ;
}

void op_manager::seek_into_trade(const trade_info_t & ti)
{
	const strike_info_t * work_strike = get_work_strike(ti);
	log4cpp::Category::getInstance(m_log_cat).info("%s: work-strike %s",
			__FUNCTION__, strike_info_txt(*work_strike).c_str());

	u_int64_t project_wedding =  work_strike->call.current + work_strike->put.current;
	project_wedding /= 2;
	log4cpp::Category::getInstance(m_log_cat).info("%s: projected wedding price = %lu",
			__FUNCTION__, project_wedding);

	u_int64_t entry_price = project_wedding + 80;
	entry_price  = congr_c_mod_m(30, 50, project_wedding + 80);
	log4cpp::Category::getInstance(m_log_cat).info("%s: designated entry price = %lu",
			__FUNCTION__, entry_price);
	//log4cpp::Category::getInstance(m_log_cat).info("%s: target exit price-1 = %lu; target exit price-2 = %lu; target exit price-3 = %lu",
			//__FUNCTION__, entry_price + 50, entry_price + 100, entry_price + 150);

	if(work_strike->call.current == entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is equal to the entry price",
				__FUNCTION__, work_strike->call.current);
	else if(work_strike->call.current < entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is lower than the entry price",
				__FUNCTION__, work_strike->call.current);
	else
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is greater than the entry price",
				__FUNCTION__, work_strike->call.current);

	if(work_strike->put.current == entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is equal to the entry price",
				__FUNCTION__, work_strike->put.current);
	else if(work_strike->put.current < entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is lower than the entry price",
				__FUNCTION__, work_strike->put.current);
	else
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is greater than the entry price",
				__FUNCTION__, work_strike->put.current);

}

const strike_info_t * op_manager::get_work_strike(const trade_info_t & ti)
{
	return ti.strikes + 4 + (((u_int64_t)ti.index)%10)/5;
}

u_int64_t op_manager::congr_c_mod_m(const u_int64_t c, const u_int64_t m, const u_int64_t x)
{
	int64_t y = c - x%m;
	return x + ((0 > y)? (m + y): y);
}
