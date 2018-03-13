
#pragma once

class informer_cb_api
{
public:
	virtual void trade_info_update(const trade_info_t &) = 0;
};

class informer : public threaded
{
private:
	informer_cb_api * m_informee;
protected:
	void inform(const trade_info_t & update) { m_informee->trade_info_update(update); }
public:
	informer() : m_informee(NULL) {}
	virtual ~informer() {}

	void set_informee(informer_cb_api * informee) { m_informee = informee; }

	virtual int init(const std::string & log_cat, const informer_conf * conf) = 0;
};
