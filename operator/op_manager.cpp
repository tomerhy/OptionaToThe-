
#include <stdlib.h>
#include <semaphore.h>

#include <log4cpp/Category.hh>

#include <string>

#include "config.h"
#include "record.h"
#include "threaded.h"
#include "informer.h"
#include "op_manager.h"
#include "dreamer.h"

op_manager::op_manager()
: m_processed(true), m_informer(NULL)
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
	m_informer->set_informee(this);
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
	log4cpp::Category::getInstance(m_log_cat).notice("%s: processing new record update.", __FUNCTION__);
}
