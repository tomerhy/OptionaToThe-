
#include <stdlib.h>
#include <string>
#include <sstream>

#include "record.h"

std::string price_info::as_txt() const
{
	std::stringstream ss;
	ss << "[current=" << current << "; base=" << base << "; low=" << low << "; high=" << high << "]";
	return ss.str();
}

std::string strike_info::as_txt() const
{
	std::stringstream ss;
	ss << "[strike=" << strike_value << "; call=" << call.as_txt() << "; put=" << put.as_txt() << "]";
	return ss.str();
}

std::string trade_info::as_txt() const
{
	std::stringstream ss;
	ss << "[index=" << index << "; change=" << change << "; stddev=" << stddev << "; ";
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
		ss << strikes[i].as_txt();
	ss << "]";
	return ss.str();
}
