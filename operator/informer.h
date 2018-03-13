
#pragma once

class infomer_cb_api
{
public:
	virtual void trade_info_update(const trade_info_t &) = 0;
};
