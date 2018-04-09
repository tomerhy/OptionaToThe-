
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

//*********************************************************************************//
trade_info::trade_info()
: record_base(trade_info_record), m_index(0), m_change(0), m_stddev(0)
{
}

trade_info::trade_info(const trade_info & other)
: record_base(trade_info_record), m_index(other.m_index), m_change(other.m_change), m_stddev(other.m_stddev)
{
}

double trade_info::get_index() const
{
	return m_index;
}

double trade_info::get_change() const
{
	return m_change;
}

double trade_info::get_stddev() const
{
	return m_stddev;
}

void trade_info::set_index(const double index)
{
	m_index = index;
}

void trade_info::set_change(const double change)
{
	m_change = change;
}

void trade_info::set_stddev(const double stddev)
{
	m_stddev = stddev;
}

std::string trade_info::as_txt() const
{
	std::stringstream ss;
	ss << "[index=" << m_index << "; change=" << m_change << "; stddev=" << m_stddev << "; ";
	for(size_t i = 0; i < STRIKE_INFO_SIZE; ++i)
		ss << strikes[i].as_txt();
	ss << "]";
	return ss.str();
}
//*********************************************************************************//
trade_request::trade_request()
: record_base(trade_request_record), m_id(0)
{
}

trade_request::trade_request(const trade_request & other)
: record_base(trade_request_record), m_id(other.m_id)
{
}

int trade_request::get_id() const
{
	return m_id;
}

void trade_request::set_id(const int id)
{
	m_id = id;
}

std::string trade_request::as_txt() const
{
	std::stringstream ss;
	ss << "[id=" << m_id << "]";
	return ss.str();
}
//*********************************************************************************//
trade_result::trade_result()
: trade_request(), m_result(0)
{
}

trade_result::trade_result(const trade_result & other)
: trade_request(other), m_result(other.m_result)
{
}

int trade_result::get_result() const
{
	return m_result;
}

void trade_result::set_result(const int result)
{
	m_result = result;
}

std::string trade_result::as_txt() const
{
	std::stringstream ss;
	ss << "[result=" << m_result << "; request=" << trade_request::as_txt() << "]";
	return ss.str();
}
//*********************************************************************************//
