
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

void price_info::clear()
{
	m_current = 0;
	m_base = 0;
	m_low = 0;
	m_high = 0;
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

const price_info & price_info::operator = (const price_info & other)
{
	if(this != &other)
	{
		this->m_current = other.m_current;
		this->m_base = other.m_base;
		this->m_low = other.m_low;
		this->m_high = other.m_high;
	}
	return *this;
}
//*********************************************************************************//
strike_info::strike_info()
: m_strike_value(0)
{
}

strike_info::strike_info(const strike_info & other)
: m_strike_value(other.m_strike_value), put(other.put), call(other.call)
{
}

void strike_info::clear()
{
	m_strike_value = 0;
	put.clear();
	call.clear();
}

u_int64_t strike_info::get_strike_value() const
{
	return m_strike_value;
}

void strike_info::set_strike_value(const u_int64_t strike_value)
{
	m_strike_value = strike_value;
}

std::string strike_info::as_txt() const
{
	std::stringstream ss;
	ss << "[strike=" << m_strike_value << "; call=" << call.as_txt() << "; put=" << put.as_txt() << "]";
	return ss.str();
}

const strike_info & strike_info::operator = (const strike_info & other)
{
	if(this != &other)
	{
		this->m_strike_value = other.m_strike_value;
		this->put = other.put;
		this->call = other.call;
	}
	return *this;
}
//*********************************************************************************//
trade_info::trade_info()
: record_base(), m_index(0), m_change(0), m_stddev(0)
{
}

trade_info::trade_info(const trade_info & other)
: record_base(), m_index(other.m_index), m_change(other.m_change), m_stddev(other.m_stddev), strikes(other.strikes)
{
}

record_type_t trade_info::get_type() const
{
	return trade_info_record;
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
: m_id(0), m_target(trade_request::tt_nil), m_strike(0), m_price(0)
{
}

trade_request::trade_request(const trade_request & other)
: m_id(other.m_id), m_target(other.m_target), m_strike(other.m_strike), m_price(other.m_price)
{
}

record_type_t trade_request::get_type() const
{
	return trade_request_record;
}

void trade_request::clear()
{
	m_id = 0;
	m_strike = 0;
	m_price = 0;
	m_target = trade_request::tt_nil;
}

int trade_request::get_id() const
{
	return m_id;
}

void trade_request::set_id(const int id)
{
	m_id = id;
}

trade_request::trade_target_t trade_request::get_target() const
{
	return m_target;
}

void trade_request::set_target(const trade_target_t target)
{
	m_target = target;
}

u_int64_t trade_request::get_strike() const
{
	return m_strike;
}

void trade_request::set_strike(u_int64_t strike)
{
	m_strike = strike;
}

u_int64_t trade_request::get_price() const
{
	return m_price;
}

void trade_request::set_price(u_int64_t price)
{
	m_price = price;
}

std::string trade_request::as_txt() const
{
	std::stringstream ss;
	ss << "[id=" << m_id << "; target=" << target_as_txt(m_target) << "; strike=" << m_strike  << "; price=" << m_price << "]";
	return ss.str();
}

const trade_request & trade_request::operator = (const trade_request & other)
{
	if(this != &other)
	{
		this->m_id = other.m_id;
		this->m_target = other.m_target;
		this->m_strike = other.m_strike;
		this->m_price = other.m_price;
	}
	return *this;
}

std::string trade_request::target_as_txt(trade_target_t tt) const
{
	switch(tt)
	{
	case tt_nil: return "nil";
	case tt_buy_call: return "buy_call";
	case tt_buy_put: return "buy_put";
	case tt_sell_call: return "sell_call";
	case tt_sell_put: return "sell_put";
	default: return "unknown";
	}
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

trade_result::trade_result(const trade_request & original)
: trade_request(original), m_result(0)
{
}

record_type_t trade_result::get_type() const
{
	return trade_result_record;
}

void trade_result::clear()
{
	m_result = 0;
	trade_request::clear();
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

const trade_result & trade_result::operator = (const trade_result & other)
{
	if(this != &other)
	{
		*((trade_request*)this) = *((trade_request*)(&other));
		this->m_result = other.m_result;
	}
	return *this;
}
//*********************************************************************************//
