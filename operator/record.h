
#pragma once

class record_base
{
public:
	record_base() : id(0) {}
	virtual ~record_base() {}

	int id;
};

class price_info
{
public:
	u_int64_t current, base, low, high;

	std::string as_txt() const;
};

class strike_info
{
public:
	u_int64_t strike_value;
	price_info put, call;

	std::string as_txt() const;
};

#define STRIKE_INFO_SIZE 10

class trade_info : public record_base
{
public:
	double index, change, stddev;
	strike_info strikes[STRIKE_INFO_SIZE];

	std::string as_txt() const;
};

class trade_request : public record_base
{
public:

	std::string as_txt() const;
};

class trade_result : public record_base
{
public:
	int result;

	std::string as_txt() const;
};
