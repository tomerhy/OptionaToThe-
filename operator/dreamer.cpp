
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

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

typedef struct
{
	dreamer * self;
	struct event_base * the_base;
	struct event * the_dream;
}dream_prm_t;

void dream_cb(evutil_socket_t, short, void *);

const struct timeval dreamer::dream_event_timeout = { 0 , 50000 };

void dreamer::run()
{
	dream_prm_t arg;
	arg.self = this;
	arg.the_base = event_base_new();
	arg.the_dream = event_new(arg.the_base, -1, EV_TIMEOUT, dream_cb, &arg);

	event_add(arg.the_dream, &dreamer::dream_event_timeout);
	event_base_dispatch(arg.the_base);
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

void setup_connection(dream_prm_t * drm);
int local_bind(int sock, dream_prm_t * drm);
void check_connection(dream_prm_t * drm);

void dream_cb(evutil_socket_t sock, short what, void * arg)
{
	dream_prm_t * drm = (dream_prm_t *)arg;
	event_free(drm->the_dream);

	if(0 != drm->self->still_running())
	{
		if(0 <= sock) close(sock);
		event_base_loopbreak(drm->the_base);
		return;
	}

	if(0 > sock)
		setup_connection(drm);
	else
		check_connection(drm);
}

void setup_connection(dream_prm_t * drm)
{
	int conn_sock = -1;
	if (0 <= (conn_sock = socket(AF_INET, SOCK_STREAM, 0)))
	{
		log4cpp::Category::getInstance(drm->self->get_log_cat()).debug("%s: socket() created %d.", __FUNCTION__, conn_sock);
		if(0 == local_bind(conn_sock, drm))
		{
			log4cpp::Category::getInstance(drm->self->get_log_cat()).debug("%s: local binding of [%s:%hu].",
					__FUNCTION__, drm->self->get_locl_addr().c_str(), drm->self->get_locl_port());

			struct sockaddr_in service_address;
			if (inet_aton(drm->self->get_srvc_addr().c_str(), &service_address.sin_addr) != 0)
			{
				service_address.sin_port = htons(drm->self->get_srvc_port());
				service_address.sin_family = AF_INET;

				if(0 == connect(conn_sock, (const struct sockaddr *)&service_address, (socklen_t)sizeof(struct sockaddr_in)))
				{
					drm->the_dream = event_new(drm->the_base, conn_sock, EV_READ|EV_TIMEOUT, dream_cb, drm);
					event_add(drm->the_dream, &dreamer::dream_event_timeout);
					return;
				}
				else
				{
			        int errcode = errno;
			        char errmsg[256];
			        log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: connect() failed with error %d : [%s].",
			        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
				}
			}
			else
			{
				log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: invalid service address [%s].",
						__FUNCTION__, drm->self->get_srvc_addr().c_str());
			}
		}
		else
		{
			log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: failed binding the local address [%s:%hu].",
					__FUNCTION__, drm->self->get_locl_addr().c_str(), drm->self->get_locl_port());
		}
		close(conn_sock);
	}
	else
	{
        int errcode = errno;
        char errmsg[256];
        log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: socket() failed with error %d : [%s].",
        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	}
	drm->the_dream = event_new(drm->the_base, -1, EV_TIMEOUT, dream_cb, drm);
	event_add(drm->the_dream, &dreamer::dream_event_timeout);
}

int local_bind(int sock, dream_prm_t * drm)
{
	if(drm->self->get_locl_addr().empty() && 0 == drm->self->get_locl_port())
		return 0;

	std::string loc_addr = drm->self->get_locl_addr();
	if(loc_addr.empty())
		loc_addr = "0.0.0.0";

	struct sockaddr_in local_address;
	if (0 != inet_aton(loc_addr.c_str(), &local_address.sin_addr))
	{
		local_address.sin_port = htons(drm->self->get_locl_port());
		local_address.sin_family = AF_INET;

		if(0 != bind(sock, (const struct sockaddr *)&local_address, (socklen_t)sizeof(struct sockaddr_in)))
		{
			log4cpp::Category::getInstance(drm->self->get_log_cat()).debug("%s: bind() of %d to [%s:%hu].",
					__FUNCTION__, sock, drm->self->get_locl_addr().c_str(), drm->self->get_locl_port());
			return 0;
		}
		else
		{
	        int errcode = errno;
	        char errmsg[256];
	        log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: bind() failed with error %d : [%s].",
	        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		}
	}
	else
	{
		log4cpp::Category::getInstance(drm->self->get_log_cat()).error("%s: invalid local address [%s].",
				__FUNCTION__, loc_addr.c_str());
	}
	return -1;
}

/*
 *
 *
 *
 */
