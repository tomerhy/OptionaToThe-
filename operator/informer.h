
#pragma once

class informer_conf;

#include "record.h"

class informer_cb_api
{
public:
	virtual void trade_info_update(const trade_info_t &) = 0;
};

class informer
{
private:
	informer_cb_api * m_informee;
protected:
	void inform(const trade_info_t & update) { m_informee->trade_info_update(update); }
public:
	informer() : m_informee(NULL) {}
	virtual ~informer() {}

	void set_informee(informer_cb_api * informee) { m_informee = informee; }

	virtual int init(const informer_conf * conf) = 0;
	virtual int start() = 0;
	virtual int stop() = 0;
};
