
#include <stdlib.h>
#include <deque>
#include <string>
#include <semaphore.h>

#include <log4cpp/Category.hh>

#include "config.h"
#include "record.h"
#include "threaded.h"
#include "executor.h"
#include "test_executor.h"

test_executor::test_executor()
: m_total_requests(0), m_success_requests(0)
{
	pthread_mutex_init(&m_event_lock, NULL);
	pthread_mutex_init(&m_reque_lock, NULL);
	pthread_cond_init(&m_event, NULL);
	srand(1);
}

test_executor::~test_executor()
{
	pthread_mutex_destroy(&m_event_lock);
	pthread_mutex_destroy(&m_reque_lock);
	pthread_cond_destroy(&m_event);
}

int test_executor::init(const std::string & log_cat, const executor_conf * conf)
{
	m_log_cat = log_cat + '.' + conf->log_conf->category;
	log4cpp::Category::getInstance(m_log_cat).setPriority((log4cpp::Priority::PriorityLevel)conf->log_conf->level);

	const test_executor_conf * ptec = dynamic_cast<const test_executor_conf *>(conf);
	if(NULL == ptec)
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: executor conf cast failure.", __FUNCTION__);
		return -1;
	}
	m_total_requests = ptec->total_requests;
	m_success_requests = ptec->success_requests;
	return 0;
}

void test_executor::run()
{
	static const unsigned int idle_wait_ms = 250;

	struct timespec event_timeout;

	log4cpp::Category::getInstance(m_log_cat).debug("%s: test executor is running.", __FUNCTION__);
	do
	{
		if(!process_trade_requests())
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

void test_executor::execute(const trade_request & req)
{
	log4cpp::Category::getInstance(m_log_cat).debug("%s: adding trade request %s.", __FUNCTION__, req.as_txt().c_str());
	pthread_mutex_lock(&m_reque_lock);
	m_reque.push_back(req);
	pthread_mutex_unlock(&m_reque_lock);
	pthread_cond_signal(&m_event);
}

bool test_executor::process_trade_requests()
{
	bool requested = false;
	trade_request req;

	pthread_mutex_lock(&m_reque_lock);
	if((requested = !m_reque.empty()))
	{
		req = *m_reque.begin();
		m_reque.pop_front();
	}
	pthread_mutex_unlock(&m_reque_lock);

	if(requested)
		process_trade_request(req);
	return requested;
}

void test_executor::process_trade_request(const trade_request & request)
{
	log4cpp::Category::getInstance(m_log_cat).debug("%s: processing trade request %s.", __FUNCTION__, request.as_txt().c_str());

	trade_result result(request);

	u_int64_t x = ((u_int64_t)rand())%m_total_requests;
	if(m_success_requests > x)
		result.set_result(0);
	else
		result.set_result(-1);
	this->report(result);
}
