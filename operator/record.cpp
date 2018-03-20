
#include <stdlib.h>
#include <string>
#include <sstream>

#include "record.h"

std::string price_info_txt(const price_info_t & pi)
{
	std::stringstream ss;
	ss << "[curr=" << pi.current << "; base=" << pi.base << "; low=" << pi.low << "; high=" << pi.high << "]";
	return ss.str();
}

std::string strike_info_txt(const strike_info_t & si)
{
	std::stringstream ss;
	ss << "[strike=" << si.strike_value << "; call=" << price_info_txt(si.call) << "; put=" << price_info_txt(si.put) << "]";
	return ss.str();
}

std::string trade_info_txt(const trade_info_t & ti)
{
	std::stringstream ss;
	ss << "[index=" << ti.index << "; change=" << ti.change << "; stddev=" << ti.stddev << "; ";
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
		ss << strike_info_txt(ti.strikes[i]);
	ss << "]";
	return ss.str();
}
