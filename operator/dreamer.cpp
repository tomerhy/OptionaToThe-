
#include <log4cpp/Category.hh>

#include "dreamer.h"
#include "config.h"

dreamer::dreamer()
: informer(), m_srvc_port(0), m_locl_port(0)
{
}

dreamer::~dreamer()
{
}

int dreamer::init(const std::string & log_cat, const informer_conf * conf)
{
	m_log_cat = log_cat + '.' + conf->log_conf->category;
	log4cpp::Category::getInstance(m_log_cat).setPriority((log4cpp::Priority::PriorityLevel)conf->log_conf->level);

	const dreamer_conf * dconf = dynamic_cast<const dreamer_conf *>(conf);
	if(NULL != dconf)
	{
		m_srvc_addr = dconf->srvc_addr;
		m_srvc_port = dconf->srvc_port;
		m_locl_addr = dconf->local_addr;
		m_locl_port = dconf->local_port;

		return 0;
	}
	else
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: informer configuration cast to a dreamer's failed.", __FUNCTION__);
		return -1;
	}
}

