
#pragma once

class informer_conf;

class informer_cb_api
{
public:
	virtual void trade_info_update(const trade_info_t &) = 0;
};

class informer
{
	informer_cb_api * m_informee;
public:
	informer();
	virtual ~informer();

	void set_informee(informer_cb_api * informee) { m_informee = informee; }

	virtual int init(const informer_conf * conf) = 0;
	virtual int start() = 0;
	virtual int stop() = 0;
};
