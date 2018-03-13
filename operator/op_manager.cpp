
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <log4cpp/Category.hh>

#include "op_manager.h"
#include "config.h"
#include "dreamer.h"

void * manager_proc(void * arg)
{
	((op_manager*)arg)->run();
	return NULL;
}

op_manager::op_manager()
: m_proc_thread(0), m_processed(true), m_informer(NULL)
{
	sem_init(&m_run_flag, 0, 0);
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

	sem_destroy(&m_run_flag);
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

	if(0 != m_informer->init(conf->info_conf))
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: informer init() failed.", __FUNCTION__);
		return -1;
	}

	return 0;
}

int op_manager::start()
{
	int run_flag_value = 0, errcode = 0;
	if(0 != sem_getvalue(&m_run_flag, &run_flag_value))
	{
        errcode = errno;
    	char errmsg[256];
    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_getvalue() failed with error %d : [%s].",
    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
        return -1;
	}

	if(0 < run_flag_value)
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: positive run flag; cannot restart.", __FUNCTION__);
		return -1;
	}

	if(0 != sem_post(&m_run_flag))
	{
        errcode = errno;
    	char errmsg[256];
    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_post() failed with error %d : [%s].",
    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
        return -1;
	}

	if(0 != m_proc_thread)
		log4cpp::Category::getInstance(m_log_cat).warn("%s: thread ID is non-zero; possibly a restart.", __FUNCTION__);

	errcode = pthread_create(&m_proc_thread, NULL, manager_proc, this);
	if(errcode != 0)
	{
		char errmsg[256];
		log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_create() failed with error %d : [%s].",
				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		m_proc_thread = 0;

		if(0 != sem_wait(&m_run_flag))
		{
	        errcode = errno;
	    	char errmsg[256];
	    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_wait() failed with error %d : [%s].",
	    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		}
		return -1;
	}

	if(0 != m_informer->start())
	{
    	log4cpp::Category::getInstance(m_log_cat).error("%s: informer start() failed.", __FUNCTION__);
    	stop();
    	return -1;
	}

	log4cpp::Category::getInstance(m_log_cat).info("%s: started.", __FUNCTION__);
	return 0;
}

int op_manager::stop()
{
	if(0 != m_informer->stop())
    	log4cpp::Category::getInstance(m_log_cat).error("%s: informer stop() failed.", __FUNCTION__);

	int errcode;
	if(0 != sem_wait(&m_run_flag))
	{
        errcode = errno;
    	char errmsg[256];
    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_wait() failed with error %d : [%s].",
    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	}

	if(0 != m_proc_thread)
    {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 2;//2 sec timeout for thread join

        void * p = NULL;
        errcode = pthread_timedjoin_np(m_proc_thread, &p, &timeout);
        if(0 != errcode)
        {
        	char errmsg[256];
        	log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_timedjoin_np() failed with error %d : [%s].",
        			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
        	errcode = pthread_cancel(m_proc_thread);
        	if(0 != errcode)
        		log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_cancel() failed with error %d : [%s].",
        				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
            else
            	log4cpp::Category::getInstance(m_log_cat).warn("%s: a thread cancellation request had to be issued.", __FUNCTION__);
        }
        m_proc_thread = 0;
    }
	log4cpp::Category::getInstance(m_log_cat).info("%s: stopped.", __FUNCTION__);
	return true;
}

void op_manager::run()
{
	static const unsigned int idle_wait_ms = 250;

	int run_flag_value = 0;
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

		if(sem_getvalue(&m_run_flag, &run_flag_value) != 0)
		{
	        int errcode = errno;
	    	char errmsg[256];
	    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_getvalue() failed with error %d : [%s].",
	    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	        break;
		}
	}while(0 < run_flag_value);
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
