
#pragma once

#include <stdlib.h>

typedef struct
{
	u_int64_t current, base, low, high;
}price_info_t;

typedef struct
{
	u_int64_t strike_value;
	price_info_t put, call;
}strike_info_t;

typedef struct
{
	double current, change, stddev;
	strike_info_t strikes[10];
}trade_info_t;
