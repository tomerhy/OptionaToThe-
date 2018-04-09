
#pragma once
//*********************************************************************************//
typedef enum
{
	nil_record = 0,
	trade_info_record,
	trade_request_record,
	trade_result_record,
}record_type_t;
//*********************************************************************************//
class record_base
{
	record_type_t m_type;
public:
	record_base(const record_type_t type) : m_type(type) {}
	virtual ~record_base() {}

	record_type_t get_type() const { return m_type; }
};
//*********************************************************************************//
class price_info
{
	u_int64_t m_current, m_base, m_low, m_high;
public:
	price_info();
	price_info(const price_info & other);

	u_int64_t get_current() const;
	u_int64_t get_base() const;
	u_int64_t get_low() const;
	u_int64_t get_high() const;
	void set_current(const u_int64_t);
	void set_base(const u_int64_t);
	void set_low(const u_int64_t);
	void set_high(const u_int64_t);

	std::string as_txt() const;
};
//*********************************************************************************//
class strike_info
{
	u_int64_t m_strike_value;
public:
	strike_info();
	strike_info(const strike_info & other);

	u_int64_t get_strike_value() const;
	void set_strike_value(const u_int64_t);

	price_info put, call;

	std::string as_txt() const;
};
//*********************************************************************************//
#define STRIKE_INFO_SIZE 10

class trade_info : public record_base
{
	double m_index, m_change, m_stddev;
public:
	trade_info();
	trade_info(const trade_info & other);

	double get_index() const;
	double get_change() const;
	double get_stddev() const;
	void set_index(const double);
	void set_change(const double);
	void set_stddev(const double);

	strike_info strikes[STRIKE_INFO_SIZE];

	std::string as_txt() const;
};
//*********************************************************************************//
class trade_request : public record_base
{
	int m_id;
public:
	trade_request();
	trade_request(const trade_request & other);

	int get_id() const;
	void set_id(const int id);

	std::string as_txt() const;
};
//*********************************************************************************//
class trade_result : public trade_request
{
	int m_result;
public:
	trade_result();
	trade_result(const trade_result & other);

	int get_result() const;
	void set_result(const int result);

	std::string as_txt() const;
};
//*********************************************************************************//
