
#pragma once

#include <string>

class informer_conf;

#include "informer.h"

class dreamer : public informer
{
    std::string m_log_cat;
	std::string m_srvc_addr, m_locl_addr;
	u_int16_t m_srvc_port, m_locl_port;
public:
	dreamer();
	virtual ~dreamer();

	virtual int init(const std::string & log_cat, const informer_conf * conf);
	virtual int start();
	virtual int stop();
};
