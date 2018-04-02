
#include <deque>
#include <string>
#include <semaphore.h>

#include <log4cpp/Category.hh>

#include "config.h"
#include "threaded.h"
#include "executor.h"
#include "test_executor.h"

test_executor::test_executor()
{
	pthread_mutex_init(&m_event_lock, NULL);
	pthread_mutex_init(&m_reque_lock, NULL);
	pthread_cond_init(&m_event, NULL);
}

test_executor::~test_executor()
{
	pthread_mutex_destroy(&m_event_lock);
	pthread_mutex_destroy(&m_reque_lock);
	pthread_cond_destroy(&m_event);
}

int test_executor::init(const std::string & log_cat, const executor_conf * conf)
{
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

	do
	{
		process_trade_requests();

		pthread_mutex_lock(&m_event_lock);
		clock_gettime(CLOCK_REALTIME, &event_timeout);
		event_timeout.tv_nsec += (idle_wait_ms * 1000000/*mili-to-nano*/);
		event_timeout.tv_sec += (event_timeout.tv_nsec / 1000000000);
		event_timeout.tv_nsec = (event_timeout.tv_nsec%1000000000);
		pthread_cond_timedwait(&m_event, &m_event_lock, &event_timeout);
		pthread_mutex_unlock(&m_event_lock);
	}while(0 == still_running());
}

void test_executor::execute(const trade_request_t & req)
{
	pthread_mutex_lock(&m_reque_lock);
	m_reque.push_back(req);
	pthread_mutex_unlock(&m_reque_lock);
	pthread_cond_signal(&m_event);
}
