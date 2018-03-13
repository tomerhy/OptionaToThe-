
#include <semaphore.h>

#include <log4cpp/Category.hh>
#include <event2/event.h>

#include "config.h"
#include "record.h"
#include "threaded.h"
#include "informer.h"
#include "dreamer.h"

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

typedef enum { nil = 0, sock = 1, bound = 2, conn = 3 } sock_state_t;

typedef struct
{
	dreamer * self;
	sock_state_t sock_state;
	int conn_sock;
	struct event_base * the_base;
	struct event * the_dream;
}event_prm_t;

void dream_cb(evutil_socket_t, short, void *);

void dreamer::run()
{
	event_prm_t arg;
	arg.self = this;
	arg.sock_state = nil;
	arg.conn_sock = -1;
	arg.the_base = event_base_new();
	arg.the_dream = event_new(arg.the_base, -1, EV_TIMEOUT, dream_cb, &arg);

	struct timeval _50_ms_ = { 0 , 50000 };
	event_add(arg.the_dream, &_50_ms_);

	event_free(arg.the_dream);
	event_base_free(arg.the_base);
}

std::string dreamer::get_log_cat() const
{
	return this->m_log_cat;
}

std::string dreamer::get_srvc_addr() const
{
	return this->m_srvc_addr;
}

std::string dreamer::get_locl_addr() const
{
	return this->m_locl_addr;
}

u_int16_t dreamer::get_srvc_port() const
{
	return this->m_srvc_port;
}

u_int16_t dreamer::get_locl_port() const
{
	return this->m_locl_port;
}

