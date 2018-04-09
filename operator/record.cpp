
#include <stdlib.h>
#include <string>
#include <sstream>

#include "record.h"

price_info::price_info()
: m_current(0), m_base(0), m_low(0), m_high(0)
{
}

price_info::price_info(const price_info & other)
: m_current(other.m_current), m_base(other.m_base), m_low(other.m_low), m_high(other.m_high)
{
}

u_int64_t price_info::get_current() const
{
	return m_current;
}

u_int64_t price_info::get_base() const
{
	return m_base;
}

u_int64_t price_info::get_low() const
{
	return m_low;
}

u_int64_t price_info::get_high() const
{
	return m_high;
}

void price_info::set_current(const u_int64_t current)
{
	m_current = current;
}

void price_info::set_base(const u_int64_t base)
{
	m_base = base;
}

void price_info::set_low(const u_int64_t low)
{
	m_low = low;
}

void price_info::set_high(const u_int64_t high)
{
	m_high = high;
}

std::string price_info::as_txt() const
{
	std::stringstream ss;
	ss << "[current=" << m_current << "; base=" << m_base << "; low=" << m_low << "; high=" << m_high << "]";
	return ss.str();
}
//*********************************************************************************//
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
