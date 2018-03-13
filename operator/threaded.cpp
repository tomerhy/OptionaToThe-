
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

#include <string>

#include <log4cpp/Category.hh>

#include "threaded.h"

void * thread_proc(void * arg)
{
	((threaded*)arg)->run();
	return NULL;
}

threaded::threaded()
: m_thread(0)
{
}

threaded::~threaded()
{
}

int threaded::still_running()
{
	int run_flag_value = 0;
	if(0 != sem_getvalue(&m_run_flag, &run_flag_value))
	{
		int errcode = errno;
		char errmsg[256];
		log4cpp::Category::getInstance(m_log_cat).error("%s: sem_getvalue() failed with error %d : [%s].",
				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	}
	return (0 < run_flag_value)? 0: -1;
}

int threaded::start()
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

	if(0 != m_thread)
		log4cpp::Category::getInstance(m_log_cat).warn("%s: thread ID is non-zero; possibly a restart.", __FUNCTION__);

	errcode = pthread_create(&m_thread, NULL, thread_proc, this);
	if(errcode != 0)
	{
		char errmsg[256];
		log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_create() failed with error %d : [%s].",
				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		m_thread = 0;

		if(0 != sem_wait(&m_run_flag))
		{
	        errcode = errno;
	    	char errmsg[256];
	    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_wait() failed with error %d : [%s].",
	    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		}
		return -1;
	}

	log4cpp::Category::getInstance(m_log_cat).info("%s: started.", __FUNCTION__);
	return 0;
}

int threaded::stop()
{
	int errcode;
	if(0 != sem_wait(&m_run_flag))
	{
        errcode = errno;
    	char errmsg[256];
    	log4cpp::Category::getInstance(m_log_cat).error("%s: sem_wait() failed with error %d : [%s].",
    			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	}

	if(0 != m_thread)
    {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 2;//2 sec timeout for thread join

        void * p = NULL;
        errcode = pthread_timedjoin_np(m_thread, &p, &timeout);
        if(0 != errcode)
        {
        	char errmsg[256];
        	log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_timedjoin_np() failed with error %d : [%s].",
        			__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
        	errcode = pthread_cancel(m_thread);
        	if(0 != errcode)
        		log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_cancel() failed with error %d : [%s].",
        				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
            else
            	log4cpp::Category::getInstance(m_log_cat).warn("%s: a thread cancellation request had to be issued.", __FUNCTION__);
        }
        m_thread = 0;
    }
	log4cpp::Category::getInstance(m_log_cat).info("%s: stopped.", __FUNCTION__);
	return true;
}

