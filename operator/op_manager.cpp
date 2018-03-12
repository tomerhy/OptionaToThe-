
#include <stdlib.h>
#include <string.h>

#include <log4cpp/Category.hh>

#include "op_manager.h"
#include "config.h"

void * manager_proc(void * arg)
{
	((op_manager*)arg)->run();
	return NULL;
}

op_manager::op_manager()
: m_proc_thread(0)
{
	sem_init(&m_run_flag, 0, 0);
}

op_manager::~op_manager()
{
	sem_destroy(&m_run_flag);
}

int op_manager::init(const std::string & log_cat, const manager_conf & conf)
{
	m_log_cat = log_cat + '.' + conf.log_conf->category;
	log4cpp::Category::getInstance(m_log_cat).setPriority((log4cpp::Priority::PriorityLevel)conf.log_conf->level);

	return 0;
}

int op_manager::start()
{
	size_t value = 0;
	if(0 == get_run_flag(value) && 0 < value)
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: positive run flag; cannot restart.", __FUNCTION__);
		return -1;
	}

	if(0 != inc_run_flag())
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: incrementing the run flag failed.", __FUNCTION__);
		return -1;
	}

	if(0 != m_proc_thread)
		log4cpp::Category::getInstance(m_log_cat).warn("%s: thread ID is non-zero; possibly a restart.", __FUNCTION__);

	int errcode = pthread_create(&m_proc_thread, NULL, manager_proc, this);
	if(errcode != 0)
	{
		char errmsg[256];
		log4cpp::Category::getInstance(m_log_cat).error("%s: pthread_create() failed with error [%d:%s].",
				__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		m_proc_thread = 0;
		if(0 != dec_run_flag())
			log4cpp::Category::getInstance(m_log_cat).error("%s: decrementing the run flag failed.", __FUNCTION__);
		return -1;
	}
	return 0;

}

