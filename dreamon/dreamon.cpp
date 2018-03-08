
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <iostream>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <event2/event.h>

typedef struct __data_source
{
	std::string path;

	__data_source()
	: path("")
	{}
}data_source_t;

typedef struct __service
{
	std::string address;
	u_int16_t port;
	u_int64_t timeout_ms;

	__service()
	: address("127.0.0.1"), port(20001), timeout_ms(1000)
	{}
}service_t;

typedef struct __log
{
	std::string file;
	std::string directory;
	size_t max_files;
	size_t max_size;
	int level;

	__log()
	: file("dreamon.log"), directory("./"), max_files(2), max_size(5*1024*1024), level(3)
	{}
}log_t;

typedef struct
{
	u_int64_t opt_strike;
	u_int64_t opt_put, opt_put_base, opt_put_low, opt_put_high;
	u_int64_t opt_call, opt_call_base, opt_call_low, opt_call_high;
}sample_t;

#define SAMPLES_PER_RECORD 10
typedef struct
{
	u_int64_t current;
	int64_t percentage;
	u_int64_t stddev;
	sample_t samples[SAMPLES_PER_RECORD];
}record_t;

void get_options(int argc, char *argv[], data_source_t * dsrc, service_t * svc, log_t * log);
void show_usage(const char * prog);
void init_log(const log_t & log);
void load_data(const data_source_t & dsrc);
void load_data_file(const char * file);
int load_record(std::list<std::string> & rec_lines);
int parse_rec_hdr(record_t & rec, std::string & hdr);
int parse_sample(sample_t & sam, std::string & samline);
void serve(const service_t & svc);
int start_svc(const service_t & svc, int * srvc_sock);

void signal_cb(evutil_socket_t, short, void *);
void accept_cb(evutil_socket_t, short, void *);

std::list< record_t > records;

int main(int argc, char *argv[])
{
	data_source_t dsrc;
	service_t svc;
	log_t log;
	get_options(argc, argv, &dsrc, &svc, &log);

	init_log(log);
	load_data(dsrc);
	serve(svc);
}

void get_options(int argc, char *argv[], data_source_t * dsrc, service_t * svc, log_t * log)
{
	if(argc == 1)
	{
		show_usage(argv[0]);
		exit(0);
	}
	int opt;
	while ((opt = getopt(argc, argv, "hf:a:p:t:l:c:m:s:r:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			show_usage(argv[0]);
			exit(0);
		case 'f':
			dsrc->path = optarg;
			break;
		case 'a':
			svc->address = optarg;
			break;
		case 'p':
			svc->port = atoi(optarg);
			break;
		case 't':
			svc->timeout_ms = atoi(optarg);
			break;
		case 'l':
			log->file = optarg;
			break;
		case 'c':
			log->directory = optarg;
			break;
		case 'm':
			log->max_files = atoi(optarg);
			break;
		case 's':
			log->max_size = atoi(optarg);
			break;
		case 'r':
			log->level = atoi(optarg);
			break;
		default:
			std::cerr << "Invalid program arguments." << std::endl;
			show_usage(argv[0]);
			exit(__LINE__);
		}
	}
}

void show_usage(const char * prog)
{
	std::cout << "Usage:" << std::endl;
	std::cout << prog << "   [ OPTIONS ]" << std::endl;
	std::cout << "-f   source data file" << std::endl;
	std::cout << "-a   service address" << std::endl;
	std::cout << "-p   service port" << std::endl;
	std::cout << "-t   service timeout" << std::endl;
	std::cout << "-l   log file" << std::endl;
	std::cout << "-c   log location" << std::endl;
	std::cout << "-m   max log files" << std::endl;
	std::cout << "-s   max log size" << std::endl;
	std::cout << "-r   log level [fatal=0,alert=100,critical=200,error=300,warning=400,notice=500(default),info=600,debug=700]" << std::endl;
}

void init_log(const log_t & log)
{
	static const char the_layout[] = "%d{%y-%m-%d %H:%M:%S.%l}| %-6p | %-15c | %m%n";

	std::string log_file = log.file;
	log_file.insert(0, "/");
	log_file.insert(0, log.directory);

    log4cpp::Layout * log_layout = NULL;
    log4cpp::Appender * appender = new log4cpp::RollingFileAppender("lpm.appender", log_file.c_str(), log.max_size, log.max_files);

    bool pattern_layout = false;
    try
    {
        log_layout = new log4cpp::PatternLayout();
        ((log4cpp::PatternLayout *)log_layout)->setConversionPattern(the_layout);
        appender->setLayout(log_layout);
        pattern_layout = true;
    }
    catch(...)
    {
        pattern_layout = false;
    }

    if(!pattern_layout)
    {
        log_layout = new log4cpp::BasicLayout();
        appender->setLayout(log_layout);
    }

    log4cpp::Category::getInstance("drmn").addAppender(appender);
    log4cpp::Category::getInstance("drmn").setPriority((log4cpp::Priority::PriorityLevel)log.level);
    log4cpp::Category::getInstance("drmn").notice("dreamon log start");
}

void load_data(const data_source_t & dsrc)
{
	struct stat dsrc_stat;
	if(0 == stat(dsrc.path.c_str(), &dsrc_stat))
	{
		if(S_ISREG(dsrc_stat.st_mode))
			load_data_file(dsrc.path.c_str());
		else
		{
			log4cpp::Category::getInstance("drmn.dsrc").fatal("%s: [%s] isn't a file and only source data files are supported.",
							__FUNCTION__, dsrc.path.c_str());
			exit(__LINE__);
		}
	}
	else
	{
		int errcode = errno;
		char errmsg[256];
		log4cpp::Category::getInstance("drmn.dsrc").fatal("%s: stat() of [%s] failed with error %d : [%s]",
				__FUNCTION__, dsrc.path.c_str(), errcode, strerror_r(errcode, errmsg, 256));
		exit(__LINE__);
	}
}

void load_data_file(const char * file)
{
	FILE * pf = fopen(file, "r");
	if(NULL != pf)
	{
		static const char record_token[] = "Time,Price,%,SD";
		static const size_t record_line_count = 12;
		size_t record_load_count = 0;
		char buffer[256];
		while(NULL != fgets(buffer, 256, pf))
		{
			if(NULL != strstr(buffer, record_token))
			{
				std::list<std::string> rec_lines;
				for(size_t i = 0; i < record_line_count && NULL != fgets(buffer, 256, pf); ++i)
					rec_lines.push_back(buffer);
				if(rec_lines.size() == record_line_count && 0 == load_record(rec_lines))
					record_load_count++;
			}
		}
		fclose(pf);
		log4cpp::Category::getInstance("drmn.dsrc").info("%s: %lu records loaded.",
				__FUNCTION__, record_load_count);
	}
	else
	{
		int errcode = errno;
		char errmsg[256];
		log4cpp::Category::getInstance("drmn.dsrc").fatal("%s: fopen() of [%s] failed with error %d : [%s]",
				__FUNCTION__, file, errcode, strerror_r(errcode, errmsg, 256));
		exit(__LINE__);
	}
}

int load_record(std::list<std::string> & rec_lines)
{
	static const char samples_token[] = "Base,High,Low,CALL,,PUT,Low,High,Base";

	record_t rec;
	std::list<std::string>::iterator i = rec_lines.begin();

	if(0 != parse_rec_hdr(rec, *i))
	{
		log4cpp::Category::getInstance("drmn.dsrc").error("%s: failed parsing record header [%s]",
				__FUNCTION__, i->c_str());
		return -1;
	}
	++i;

	if(NULL == strstr(i->c_str(), samples_token))
	{
		log4cpp::Category::getInstance("drmn.dsrc").error("%s: invalid samples token [%s]",
				__FUNCTION__, i->c_str());
		return -1;
	}
	++i;

	size_t sample_index = 0;
	while(rec_lines.end() != i)
	{
		if(0 != parse_sample(rec.samples[sample_index++], *i))
		{
			log4cpp::Category::getInstance("drmn.dsrc").error("%s: failed parsing sample[%d]=[%s]",
					__FUNCTION__, sample_index-1, i->c_str());
			return -1;
		}
		++i;
	}
	records.push_back(rec);
	return 0;
}

static const char comma = ',', quote = '"';

int parse_rec_hdr(record_t & rec, std::string & hdr)
{
	//DateTime,"current.dd",%%,stddev

	std::string::size_type quote1 = hdr.find(quote);
	if(std::string::npos == quote1)
		return -1;
	std::string::size_type quote2 = hdr.find(quote, quote1 + 1);
	if(std::string::npos == quote2)
		return -1;
	std::string prce = hdr.substr((quote1 + 1), quote2 - (quote1 + 1));
	prce.erase(prce.find(comma), 1);
	rec.current = (u_int64_t)(100 * strtod(prce.c_str(), NULL));

	std::string::size_type comma1 = hdr.find(comma, quote2 + 1);
	if(std::string::npos == comma1)
		return -1;
	std::string::size_type comma2 = hdr.find(comma, comma1 + 1);
	if(std::string::npos == comma2)
		return -1;
	std::string pcnt = hdr.substr((comma1 + 1), comma2 - (comma1 + 1));
	rec.percentage =  (int64_t)(100 * strtod(pcnt.c_str(), NULL));

	std::string stdv = hdr.substr(comma2 + 1);
	rec.stddev = (u_int64_t)(100 * strtod(stdv.c_str(), NULL));

	log4cpp::Category::getInstance("drmn.dsrc").debug("%s: parsed header [%lu, %ld, %lu]",
			__FUNCTION__, rec.current, rec.percentage, rec.stddev);
	return 0;
}

int parse_sample(sample_t & sam, std::string & samline)
{
	//Base,		High,	Low,	CALL,	STRIKE,		PUT,	Low,	High,	Base
	std::string::size_type stop[8], last = std::string::npos;
	for(size_t i = 0; i < 8; ++i)
	{
		last = stop[i] = samline.find(comma, last + 1);
		if(std::string::npos == last)
			return -1;
		stop[i] += 1;
	}
	sam.opt_call_base	= strtol(samline.c_str(), NULL, 10);
	sam.opt_call_high	= strtol(samline.c_str() + stop[0], NULL, 10);
	sam.opt_call_low	= strtol(samline.c_str() + stop[1], NULL, 10);
	sam.opt_call		= strtol(samline.c_str() + stop[2], NULL, 10);
	sam.opt_strike		= strtol(samline.c_str() + stop[3], NULL, 10);
	sam.opt_put			= strtol(samline.c_str() + stop[4], NULL, 10);
	sam.opt_put_low		= strtol(samline.c_str() + stop[5], NULL, 10);
	sam.opt_put_high	= strtol(samline.c_str() + stop[6], NULL, 10);
	sam.opt_put_base	= strtol(samline.c_str() + stop[7], NULL, 10);

	log4cpp::Category::getInstance("drmn.dsrc").debug("%s: parsed sample [%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu]",
			__FUNCTION__, sam.opt_call_base, sam.opt_call_high, sam.opt_call_low, sam.opt_call,
			sam.opt_strike, sam.opt_put, sam.opt_put_low, sam.opt_put_high, sam.opt_put_base);
	return 0;
}

void serve(const service_t & svc)
{
	int srvc_sock = -1;
	if(0 != start_svc(svc, &srvc_sock))
	{
		log4cpp::Category::getInstance("drmn.srvc").fatal("%s: failed starting the service.", __FUNCTION__);
		exit(__LINE__);
	}

	struct event_base * the_base = event_base_new();

	struct event * the_signal = evsignal_new(the_base, 2/*SIGINT*/, signal_cb, the_base);
	if(0 != event_add(the_signal, NULL))
	{
		log4cpp::Category::getInstance("drmn.srvc").fatal("%s: failed adding a signal event.", __FUNCTION__);
		exit(__LINE__);
	}

	struct event * the_accept = event_new(the_base, srvc_sock, EV_READ, accept_cb, the_base);
	if(0 != event_add(the_accept, NULL))
	{
		log4cpp::Category::getInstance("drmn.srvc").fatal("%s: failed adding an accept event.", __FUNCTION__);
		exit(__LINE__);
	}

	event_base_dispatch(the_base);

	event_free(the_accept);
	event_free(the_signal);
	event_base_free(the_base);
}

int start_svc(const service_t & svc, int * srvc_sock)
{
	if (0 > (*srvc_sock = socket(AF_INET, SOCK_STREAM, 0)))
    {
        int errcode = errno;
        char errmsg[256];
        log4cpp::Category::getInstance("drmn.srvc").error("%s: socket() failed with error %d : [%s].",
        		__FUNCTION__, errcode, strerror_r(errcode, errmsg, 256));
        return -1;
    }

	struct sockaddr_in service_address;
	if (inet_aton(svc.address.c_str(), &service_address.sin_addr) == 0)
    {
		log4cpp::Category::getInstance("drmn.srvc").error("%s: invalid service address [%s].",
				__FUNCTION__, svc.address.c_str());
        return -1;
    }
	service_address.sin_port = htons(svc.port);
	service_address.sin_family = AF_INET;

	if (0 != bind(*srvc_sock, (const sockaddr *)&service_address, (socklen_t)sizeof(struct sockaddr_in)))
	{
        int errcode = errno;
        char errmsg[256];
        log4cpp::Category::getInstance("drmn.srvc").error("%s: bind() to [%s:%hu] failed with error %d : [%s].",
        		__FUNCTION__, svc.address.c_str(), svc.port, errcode, strerror_r(errcode, errmsg, 256));
        close(*srvc_sock);
        *srvc_sock = -1;
        return -1;
	}

	if (0 != listen(*srvc_sock, 10))
	{
        int errcode = errno;
        char errmsg[256];
        log4cpp::Category::getInstance("drmn.srvc").error("%s: listen() on [%s:%hu] failed with error %d : [%s].",
        		__FUNCTION__, svc.address.c_str(), svc.port, errcode, strerror_r(errcode, errmsg, 256));
        close(*srvc_sock);
        *srvc_sock = -1;
        return -1;
	}

	return 0;
}
