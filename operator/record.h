
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
public:
	record_base() {}
	virtual ~record_base() {}

	virtual record_type_t get_type() const = 0;
};
//*********************************************************************************//
class price_info
{
	u_int64_t m_current, m_base, m_low, m_high;
public:
	price_info();
	price_info(const price_info & other);

	void clear();

	u_int64_t get_current() const;
	u_int64_t get_base() const;
	u_int64_t get_low() const;
	u_int64_t get_high() const;
	void set_current(const u_int64_t);
	void set_base(const u_int64_t);
	void set_low(const u_int64_t);
	void set_high(const u_int64_t);

	std::string as_txt() const;

	const price_info & operator = (const price_info &);
};
//*********************************************************************************//
class strike_info
{
	u_int64_t m_strike_value;
public:
	strike_info();
	strike_info(const strike_info & other);

	void clear();

	u_int64_t get_strike_value() const;
	void set_strike_value(const u_int64_t);

	price_info put, call;

	std::string as_txt() const;

	const strike_info & operator = (const strike_info &);
};
//*********************************************************************************//
#define STRIKE_INFO_SIZE 10

class trade_info : public record_base
{
	double m_index, m_change, m_stddev;
public:
	trade_info();
	trade_info(const trade_info & other);

	virtual record_type_t get_type() const;

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
public:
	typedef enum { tt_nil = 0, tt_buy_call, tt_buy_put, tt_sell_call, tt_sell_put } trade_target_t;

private:
	int m_id;
	strike_info m_strike;
	trade_target_t m_target;
public:
	trade_request();
	trade_request(const trade_request & other);

	virtual record_type_t get_type() const;

	void clear();

	int get_id() const;
	void set_id(const int id);

	const strike_info & get_strike() const;
	void set_strike(const strike_info &);

	trade_target_t get_target() const;
	void set_target(const trade_target_t);

	std::string as_txt() const;
};
//*********************************************************************************//
class trade_result : public trade_request
{
	int m_result;
public:
	trade_result();
	trade_result(const trade_result & other);

	virtual record_type_t get_type() const;

	void clear();

	int get_result() const;
	void set_result(const int result);

	std::string as_txt() const;
};
//*********************************************************************************//
