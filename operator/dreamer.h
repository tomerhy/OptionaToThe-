
#pragma once

class dreamer : public informer
{
private:
    std::string m_log_cat;
	std::string m_srvc_addr, m_locl_addr;
	u_int16_t m_srvc_port, m_locl_port;

	void run();
public:
	dreamer();
	virtual ~dreamer();

	virtual int init(const std::string & log_cat, const informer_conf * conf);

    std::string get_log_cat() const;
	std::string get_srvc_addr() const;
	std::string get_locl_addr() const;
	u_int16_t get_srvc_port() const;
	u_int16_t get_locl_port() const;

	static const struct timeval dream_event_timeout;
};
