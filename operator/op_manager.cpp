
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
: m_informer(NULL), m_executor(NULL)
, m_balance(0.0), m_pnl(0.0), m_position(op_manager::mark)
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
		if(!process_record())
		{
			pthread_mutex_lock(&m_event_lock);
			clock_gettime(CLOCK_REALTIME, &event_timeout);
			event_timeout.tv_nsec += (idle_wait_ms * 1000000/*mili-to-nano*/);
			event_timeout.tv_sec += (event_timeout.tv_nsec / 1000000000);
			event_timeout.tv_nsec = (event_timeout.tv_nsec%1000000000);
			pthread_cond_timedwait(&m_event, &m_event_lock, &event_timeout);
			pthread_mutex_unlock(&m_event_lock);
		}
	}while(0 == still_running());
}

void op_manager::trade_info_update(const trade_info & update)
{
	trade_info * p = new trade_info(update);
	pthread_mutex_lock(&m_record_lock);
	m_record_queue.push_back(p);
	pthread_mutex_unlock(&m_record_lock);
	pthread_cond_signal(&m_event);
}

void op_manager::on_trade_result(const trade_result & res)
{
	trade_result * p = new trade_result(res);
	pthread_mutex_lock(&m_record_lock);
	m_record_queue.push_back(p);
	pthread_mutex_unlock(&m_record_lock);
	pthread_cond_signal(&m_event);
}

bool op_manager::process_record()
{
	record_base * prec = NULL;
	pthread_mutex_lock(&m_record_lock);
	if(!m_record_queue.empty())
	{
		prec = *m_record_queue.begin();
		m_record_queue.pop_front();
	}
	pthread_mutex_unlock(&m_record_lock);

	if(NULL != prec)
	{
		process_record(prec);
		delete prec;
		return true;
	}
	return false;
}

void op_manager::process_record(const record_base * prec)
{
	switch(prec->get_type())
	{
	case trade_info_record:
		{
			const trade_info * tirec = dynamic_cast<const trade_info *>(prec);
			if(NULL != tirec)
				process_trade_info_record(*tirec);
			else
				log4cpp::Category::getInstance(m_log_cat).warn("%s: trade info record cast failure.", __FUNCTION__);
		}
		break;
	case trade_result_record:
		{
			const trade_result * trrec = dynamic_cast<const trade_result *>(prec);
			if(NULL != trrec)
				process_trade_result_record(*trrec);
			else
				log4cpp::Category::getInstance(m_log_cat).warn("%s: trade result record cast failure.", __FUNCTION__);
		}
		break;
	default:
		break;
	}
}

void op_manager::process_trade_info_record(const trade_info & tirec)
{
	log4cpp::Category::getInstance(m_log_cat).info("%s: processing new record update.", __FUNCTION__);
	log4cpp::Category::getInstance(m_log_cat).debug(tirec.as_txt());

	if(0 != valid_record(tirec))
	{
		log4cpp::Category::getInstance(m_log_cat).warn("%s: invalid record dropped.", __FUNCTION__);
		return;
	}

	switch(m_position)
	{
	case op_manager::mark:
		log4cpp::Category::getInstance(m_log_cat).debug("%s: mark trade position; seek into trade.", __FUNCTION__);
		seek_into_trade(tirec);
		break;
	case op_manager::go:
		log4cpp::Category::getInstance(m_log_cat).debug("%s: go trade position; seek out of trade.", __FUNCTION__);
		seek_out_of_trade(tirec);
		break;
	case op_manager::set:
		log4cpp::Category::getInstance(m_log_cat).debug("%s: set trade position; drop trade info.", __FUNCTION__);
		break;
	default:
		log4cpp::Category::getInstance(m_log_cat).error("%s: unsupported trade position %d.", __FUNCTION__, (int)m_position);
		break;
	}
}

int op_manager::valid_record(const trade_info & ti)
{
	//this loop checks that all strikes' call & put
	//prices are divisible by 10 without remainder
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
		if((0 != ti.strikes[i].call.get_current()%10) || (0 != ti.strikes[i].put.get_current()%10))
			return -1;

	return 0;
}

void op_manager::seek_out_of_trade(const trade_info & ti)
{
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
	{
		if(m_go_hold.get_strike().get_strike_value() == ti.strikes[i].get_strike_value())
		{
			u_int64_t current_strike_price, purchase_strike_price;
			switch(m_go_hold.get_target())
			{
			case trade_request::tt_buy_call:
				current_strike_price = ti.strikes[i].call.get_current();
				purchase_strike_price = m_go_hold.get_strike().call.get_current();
				log4cpp::Category::getInstance(m_log_cat).info("%s: current=%lu; purchase=%lu;", __FUNCTION__, current_strike_price, purchase_strike_price);
				if((current_strike_price >= (purchase_strike_price + 50)) || (current_strike_price < purchase_strike_price))
				{
					trade_request request;
					request.set_id(time(NULL)%86400);
					request.set_strike(ti.strikes[i]);
					request.set_target(trade_request::tt_sell_call);
					this->m_executor->execute(request);
					m_position = set;
				}
				return;
			case trade_request::tt_buy_put:
				current_strike_price = ti.strikes[i].put.get_current();
				purchase_strike_price = m_go_hold.get_strike().put.get_current();
				log4cpp::Category::getInstance(m_log_cat).info("%s: current=%lu; purchase=%lu;", __FUNCTION__, current_strike_price, purchase_strike_price);
				if((current_strike_price >= (purchase_strike_price + 50)) || (current_strike_price < purchase_strike_price))
				{
					trade_request request;
					request.set_id(time(NULL)%86400);
					request.set_strike(ti.strikes[i]);
					request.set_target(trade_request::tt_sell_put);
					this->m_executor->execute(request);
					m_position = set;
				}
				return;
			default:
				log4cpp::Category::getInstance(m_log_cat).error("%s: invalid go hold trade result target %d", __FUNCTION__, (int)m_go_hold.get_target());
				return;
			}
		}
	}
	return;
}

void op_manager::seek_into_trade(const trade_info & ti)
{
	const strike_info * work_strike = get_work_strike(ti);
	log4cpp::Category::getInstance(m_log_cat).info("%s: work-strike %s",
			__FUNCTION__, work_strike->as_txt().c_str());

	u_int64_t project_wedding =  work_strike->call.get_current() + work_strike->put.get_current();
	project_wedding /= 2;
	log4cpp::Category::getInstance(m_log_cat).info("%s: projected wedding price = %lu",
			__FUNCTION__, project_wedding);

	u_int64_t entry_price = congr_c_mod_m(30, 50, project_wedding + 80);
	log4cpp::Category::getInstance(m_log_cat).info("%s: designated entry price = %lu",
			__FUNCTION__, entry_price);
	//log4cpp::Category::getInstance(m_log_cat).info("%s: target exit price-1 = %lu; target exit price-2 = %lu; target exit price-3 = %lu",
			//__FUNCTION__, entry_price + 50, entry_price + 100, entry_price + 150);

	if(m_balance < entry_price)
	{
		log4cpp::Category::getInstance(m_log_cat).info("%s: entry price %lu is higher than the current balance %.02f; can't afford the trade.",
				__FUNCTION__, entry_price, m_balance);
		return;
	}

	if(work_strike->call.get_current() == entry_price)
	{
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is equal to the entry price; requesting trade.",
				__FUNCTION__, work_strike->call.get_current());

		trade_request request;
		request.set_id(time(NULL)%86400);
		request.set_strike(*work_strike);
		request.set_target(trade_request::tt_buy_call);
		this->m_executor->execute(request);
		m_position = set;
		return;
	}
	else if(work_strike->call.get_current() < entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is lower than the entry price",
				__FUNCTION__, work_strike->call.get_current());
	else
		log4cpp::Category::getInstance(m_log_cat).info("%s: call price %lu is greater than the entry price",
				__FUNCTION__, work_strike->call.get_current());

	if(work_strike->put.get_current() == entry_price)
	{
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is equal to the entry price; requesting trade.",
				__FUNCTION__, work_strike->put.get_current());

		trade_request request;
		request.set_id(time(NULL)%86400);
		request.set_strike(*work_strike);
		request.set_target(trade_request::tt_buy_put);
		this->m_executor->execute(request);
		m_position = set;
		return;
	}
	else if(work_strike->put.get_current() < entry_price)
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is lower than the entry price",
				__FUNCTION__, work_strike->put.get_current());
	else
		log4cpp::Category::getInstance(m_log_cat).info("%s: put price %lu is greater than the entry price",
				__FUNCTION__, work_strike->put.get_current());

}

const strike_info * op_manager::get_work_strike(const trade_info & ti)
{
	return ti.strikes + 4 + (((u_int64_t)ti.get_index())%10)/5;
}

u_int64_t op_manager::congr_c_mod_m(const u_int64_t c, const u_int64_t m, const u_int64_t x)
{
	int64_t y = c - x%m;
	return x + ((0 > y)? (m + y): y);
}

void op_manager::process_trade_result_record(const trade_result & trrec)
{
	log4cpp::Category::getInstance(m_log_cat).notice("%s: trade result %s", __FUNCTION__, trrec.as_txt().c_str());

	bool trade_success = (0 == trrec.get_result())? true: false;
	switch(trrec.get_target())
	{
	case trade_request::tt_buy_call:
		if(trade_success)
		{
			m_balance -= trrec.get_strike().call.get_current();
			m_pnl -= trrec.get_strike().call.get_current();
			m_go_hold = trrec;
			m_position = go;
		}
		else
		{
			m_position = mark;
		}
		break;
	case trade_request::tt_buy_put:
		if(trade_success)
		{
			m_balance -= trrec.get_strike().put.get_current();
			m_pnl -= trrec.get_strike().put.get_current();
			m_go_hold = trrec;
			m_position = go;
		}
		else
		{
			m_position = mark;
		}
		break;
	case trade_request::tt_sell_call:
		if(trade_success)
		{
			m_balance += trrec.get_strike().call.get_current();
			m_pnl += trrec.get_strike().call.get_current();
			m_go_hold.clear();
			m_position = mark;
		}
		else
		{
			m_position = go;
		}
		break;
	case trade_request::tt_sell_put:
		if(trade_success)
		{
			m_balance += trrec.get_strike().put.get_current();
			m_pnl += trrec.get_strike().put.get_current();
			m_go_hold.clear();
			m_position = mark;
		}
		else
		{
			m_position = go;
		}
		break;
	default:
		log4cpp::Category::getInstance(m_log_cat).error("%s: invalid trade result target %d", __FUNCTION__, (int)trrec.get_target());
		break;
	}
	log4cpp::Category::getInstance(m_log_cat).notice("%s: balance %.02f; P&L %.02f;", __FUNCTION__, m_balance, m_pnl);
}
