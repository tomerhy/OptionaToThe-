
#pragma once

class dreamer : public informer
{
private:
	static const struct timeval dream_event_timeout;

    std::string m_log_cat;
	std::string m_srvc_addr, m_locl_addr;
	u_int16_t m_srvc_port, m_locl_port;

	struct event_base * the_base;
	struct event * the_dream;

	int establish_connection(int & sockfd);
	int bind_local_address(int & sockfd);
	int do_read(int & sockfd);

	void run();
public:
	dreamer();
	virtual ~dreamer();

	virtual int init(const std::string & log_cat, const informer_conf * conf);

	void on_event(int sockfd, u_int16_t what);
};
