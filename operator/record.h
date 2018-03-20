
#pragma once

typedef struct
{
	u_int64_t current, base, low, high;
}price_info_t;

std::string price_info_txt(const price_info_t &);

typedef struct
{
	u_int64_t strike_value;
	price_info_t put, call;
}strike_info_t;

std::string strike_info_txt(const strike_info_t &);

#define STRIKE_INFO_SIZE 10
typedef struct
{
	double index, change, stddev;
	strike_info_t strikes[STRIKE_INFO_SIZE];
}trade_info_t;

std::string trade_info_txt(const trade_info_t &);
