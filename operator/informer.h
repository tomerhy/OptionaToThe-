
#pragma once

class informer_cb_api
{
public:
	virtual void trade_info_update(const trade_info &) = 0;
};

class informer : public threaded
{
private:
	informer_cb_api * m_manager;
protected:
	void inform(const trade_info & update) { m_manager->trade_info_update(update); }
public:
	informer() : m_manager(NULL) {}
	virtual ~informer() {}

	void set_manager(informer_cb_api * manager) { m_manager = manager; }

	virtual int init(const std::string & log_cat, const informer_conf * conf) = 0;
};
