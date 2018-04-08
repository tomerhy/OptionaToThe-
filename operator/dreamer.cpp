
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>

#include <log4cpp/Category.hh>
#include <event2/event.h>

#include "config.h"
#include "record.h"
#include "threaded.h"
#include "informer.h"
#include "dreamer.h"

const struct timeval dreamer::dream_event_timeout = { 0 , 50000 };

void dream_cb(evutil_socket_t fd, short what, void *arg)
{
	((dreamer *)arg)->on_event(fd, what);
}

dreamer::dreamer()
: informer(), m_srvc_port(0), m_locl_port(0), the_base(NULL), the_dream(NULL)
{
	the_base = event_base_new();
	the_dream = event_new(the_base, -1, EV_TIMEOUT, dream_cb, this);
}

dreamer::~dreamer()
{
	event_free(the_dream);
	the_dream = NULL;
	event_base_free(the_base);
	the_base = NULL;
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
		event_add(the_dream, &dreamer::dream_event_timeout);
		return 0;
	}
	else
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: informer configuration cast to a dreamer's failed.", __FUNCTION__);
	}
	return -1;
}

void dreamer::run()
{
	event_base_dispatch(the_base);
}

void dreamer::on_event(int sockfd, u_int16_t what)
{
	if(0 != still_running())
	{
		if(0 <= sockfd) { close(sockfd); sockfd = -1; }
		event_base_loopbreak(the_base);
		return;
	}

	if(0 <= sockfd)
	{
		if(what & EV_READ)
		{
			do_read(sockfd);
		}
	}
	else
	{
		establish_connection(sockfd);
	}

	event_free(the_dream);
	the_dream = event_new(the_base, sockfd, EV_TIMEOUT | ((0 <= sockfd)? EV_READ: 0), dream_cb, this);
	event_add(the_dream, &dreamer::dream_event_timeout);
}

int dreamer::establish_connection(int & sockfd)
{
	if (0 <= (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		if(0 == bind_local_address(sockfd))
		{
			struct sockaddr_in service_address;
			if (0 != inet_aton(m_srvc_addr.c_str(), &service_address.sin_addr))
			{
				service_address.sin_port = htons(m_srvc_port);
				service_address.sin_family = AF_INET;

				if(0 == connect(sockfd, (const struct sockaddr *)&service_address, (socklen_t)sizeof(struct sockaddr_in)))
				{
					log4cpp::Category::getInstance(m_log_cat).debug("%s: connect() succeeded to [%s:%hu].",
								        		__FUNCTION__, m_srvc_addr.c_str(), m_srvc_port);
					return 0;
				}
				else
				{
			        int errcode = errno;
			        char errmsg[256];
			        log4cpp::Category::getInstance(m_log_cat).error("%s: connect() failed with error %d : [%s].",
			        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
				}
			}
			else
			{
				log4cpp::Category::getInstance(m_log_cat).error("%s: invalid service address [%s].",
						__FUNCTION__, m_srvc_addr.c_str());
			}
		}
		else
		{
			log4cpp::Category::getInstance(m_log_cat).error("%s: failed binding local address.",
					__FUNCTION__);
		}
		close(sockfd);
		sockfd = -1;
	}
	else
	{
        int errcode = errno;
        char errmsg[256];
        log4cpp::Category::getInstance(m_log_cat).error("%s: socket() failed with error %d : [%s].",
        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
	}
	return -1;
}

int dreamer::bind_local_address(int & sockfd)
{
	if(m_locl_addr.empty() && 0 == m_locl_port)
		return 0;

	std::string loc_addr = m_locl_addr;
	if(loc_addr.empty())
		loc_addr = "0.0.0.0";

	struct sockaddr_in local_address;
	if (0 != inet_aton(loc_addr.c_str(), &local_address.sin_addr))
	{
		local_address.sin_port = htons(m_locl_port);
		local_address.sin_family = AF_INET;

		if(0 != bind(sockfd, (const struct sockaddr *)&local_address, (socklen_t)sizeof(struct sockaddr_in)))
		{
			log4cpp::Category::getInstance(m_log_cat).debug("%s: bind() of %d to [%s:%hu].",
					__FUNCTION__, sockfd, loc_addr.c_str(), m_locl_port);
			return 0;
		}
		else
		{
	        int errcode = errno;
	        char errmsg[256];
	        log4cpp::Category::getInstance(m_log_cat).error("%s: bind() failed with error %d : [%s].",
	        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		}
	}
	else
	{
		log4cpp::Category::getInstance(m_log_cat).error("%s: invalid local address [%s].",
				__FUNCTION__, loc_addr.c_str());
	}
	return -1;

}

int dreamer::do_read(int & sockfd)
{
	typedef struct
	{
		u_int64_t opt_strike;
		u_int64_t opt_put, opt_put_base, opt_put_low, opt_put_high;
		u_int64_t opt_call, opt_call_base, opt_call_low, opt_call_high;
	}sample_t;

	#define SAMPLES_PER_RECORD 10
	typedef struct
	{
		u_int64_t current;
		int64_t percentage;
		u_int64_t stddev;
		sample_t samples[SAMPLES_PER_RECORD];
	}record_t;

	assert(SAMPLES_PER_RECORD == STRIKE_INFO_SIZE);

	record_t record;
	ssize_t readn = recv(sockfd, &record, sizeof(record_t), MSG_WAITALL);
	if(0 < readn)
	{
		if(sizeof(record_t) == readn)
		{
			trade_info ti;
			ti.index = record.current;
			ti.index /= 100;
			ti.change = record.percentage;
			ti.change /= 100;
			ti.stddev = record.stddev;
			ti.stddev /= 100;
			for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
			{
				ti.strikes[i].call.base = record.samples[i].opt_call_base;
				ti.strikes[i].call.high = record.samples[i].opt_call_high;
				ti.strikes[i].call.low = record.samples[i].opt_call_low;
				ti.strikes[i].call.current = record.samples[i].opt_call;

				ti.strikes[i].put.base = record.samples[i].opt_put_base;
				ti.strikes[i].put.high = record.samples[i].opt_put_high;
				ti.strikes[i].put.low = record.samples[i].opt_put_low;
				ti.strikes[i].put.current = record.samples[i].opt_put;

				ti.strikes[i].strike_value = record.samples[i].opt_strike;
			}
			this->inform(ti);
		}
		else
		{
	        log4cpp::Category::getInstance(m_log_cat).warn("%s: recv() returned partial record size %u; disconnecting.",
	        		__FUNCTION__, readn);
			close(sockfd);
			sockfd = -1;
		}
	}
	else
	{
		if(0 > readn)
		{
	        int errcode = errno;
	        char errmsg[256];
	        log4cpp::Category::getInstance(m_log_cat).error("%s: recv() failed with error %d : [%s].",
	        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
		}
		else
		{
	        log4cpp::Category::getInstance(m_log_cat).notice("%s: recv() returned 0 bytes; the service was closed.",
	        		__FUNCTION__, readn);
		}
		close(sockfd);
		sockfd = -1;
	}
}
