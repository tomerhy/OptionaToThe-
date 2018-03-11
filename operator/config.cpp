
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"

#include <libxml/xmlreader.h>

int load_valid(const char * config_file, xmlDocPtr & xml_doc);
int validate_doc(xmlDocPtr xml_doc, std::list<std::string> * validation_errors);
std::string get_node_attribute_value(xmlNodePtr node, const char * attribute_name);
void xml_schema_validity_error_cb(void* ctx, const char* msg, ...);
void xml_schema_validity_warning_cb(void* ctx, const char* msg, ...);
const char * get_error_level_name(xmlErrorLevel level);
void xml_structured_error_cb(void* userData, xmlErrorPtr xml_error);

int get_operator_conf(xmlDocPtr xml_doc, operator_conf & conf);
int get_log_global_conf(xmlNodePtr log_node, logger_global_conf & conf);
int get_manager_conf(xmlNodePtr manager_node, manager_conf & conf);
int load_log_level(const char * level_txt);
int get_log_basic_conf(xmlNodePtr log_node, logger_base_conf & conf);
int get_dreamer_conf(xmlNodePtr dreamer_node, dreamer_conf & conf);
int get_executor_conf(xmlNodePtr executor_node, executor_conf & conf);

int load_config(const char * config_file, operator_conf & conf)
{
	xmlDocPtr xml_doc = NULL;
	if(0 == load_valid(config_file, xml_doc))
	{
		get_operator_conf(xml_doc, conf);
		xmlFreeDoc(xml_doc);
		return 0;
	}
	else
		return -1;
}

int load_valid(const char * config_file, xmlDocPtr & xml_doc)
{
	struct stat conf_file_stat;
	if(stat(config_file, &conf_file_stat) == 0)
	{
		xml_doc = xmlParseFile(config_file);
		if(NULL != xml_doc)
		{
			std::list<std::string> validation_errors;
			if(0 == validate_doc(xml_doc, &validation_errors))
			{
				return 0;
			}
			else
			{
				std::cerr << __FUNCTION__ << ": failed to validate configuration file [" << config_file << "] for the following errors:" << std::endl;
				for(std::list<std::string>::const_iterator i = validation_errors.begin(); i != validation_errors.end(); ++i)
				{
					std::cerr << *i << std::endl;
				}
			}
			xmlFreeDoc(xml_doc);
		}
		else
			std::cerr << __FUNCTION__ << ": failed to parse configuration file [" << config_file << "]" << std::endl;
	}
	else
		std::cerr << __FUNCTION__ << ": failed to find configuration file [" << config_file << "]" << std::endl;
	return -1;
}

int validate_doc(xmlDocPtr xml_doc, std::list<std::string> * validation_errors)
{
	int result = -1;
	xmlNodePtr root = xmlDocGetRootElement(xml_doc);
	if(root != NULL)
	{
		std::string schema_file = get_node_attribute_value(root, "noNamespaceSchemaLocation");
		if(schema_file.empty() == false)
		{
			struct stat schema_file_stat;
			if(0 == stat(schema_file.c_str(), &schema_file_stat))
			{
				xmlSchemaParserCtxtPtr ctxt = xmlSchemaNewParserCtxt(schema_file.c_str());
				if(ctxt != NULL)
				{
					xmlSchemaPtr schema = xmlSchemaParse(ctxt);
					if(schema != NULL)
					{
						xmlSchemaValidCtxtPtr svctxt = xmlSchemaNewValidCtxt(schema);
						if(svctxt != NULL)
						{
							xmlSchemaSetValidErrors(svctxt, xml_schema_validity_error_cb, xml_schema_validity_warning_cb, (void*)validation_errors);
							xmlSchemaSetValidStructuredErrors(svctxt, xml_structured_error_cb, (void*)validation_errors);
							result = xmlSchemaValidateDoc(svctxt, xml_doc);
							xmlSchemaFreeValidCtxt(svctxt);
						}
						xmlSchemaFree(schema);
					}
					xmlSchemaFreeParserCtxt(ctxt);
				}
			}
		}
	}
	return result;
}

std::string get_node_attribute_value(xmlNodePtr node, const char * attribute_name)
{
	for(xmlAttrPtr p = node->properties; p != NULL; p = p->next)
	{
		if(strcmp(attribute_name, (const char*)p->name) == 0)
		{
			if(p->children && p->children->type == XML_TEXT_NODE && p->children->content != NULL)
			{
				return (const char*)p->children->content;
			}
		}
	}
	return "";
}

void xml_schema_validity_warning_cb(void* ctx, const char * msg, ...)
{
	std::list<std::string> * errors = (std::list<std::string>*)ctx;
	if(errors != NULL)
	{
		char* buffer = (char*)alloca(2048);
		va_list arglist;
		va_start(arglist, msg);
		vsnprintf(buffer, 2048, msg, arglist);
		va_end(arglist);

		errors->push_back(std::string("WARNING: ") + buffer);
	}
}

void xml_schema_validity_error_cb(void* ctx, const char* msg, ...)
{
	std::list<std::string>* errors = (std::list<std::string>*)ctx;
	if(errors != NULL)
	{
		char* buffer = (char*)alloca(2048);
		va_list arglist;
		va_start(arglist, msg);
		vsnprintf(buffer, 2048, msg, arglist);
		va_end(arglist);

		errors->push_back(std::string("ERROR: ") + buffer);
	}
}

const char * get_error_level_name(xmlErrorLevel level)
{
	switch(level)
	{
	case XML_ERR_WARNING:
		return "WARNING";
	case XML_ERR_ERROR:
		return "ERROR";
	case XML_ERR_FATAL:
		return "FATAL";
	default:
		return "NONE";
	}
}

void xml_structured_error_cb(void* userData, xmlErrorPtr xml_error)
{
	std::list<std::string> * errors = (std::list<std::string>*)userData;
	if(errors != NULL)
	{
		std::string error_txt = std::string("[")
							  + get_error_level_name(xml_error->level )
							  + ": "
							  + xml_error->message
							  + " @"
							  + xml_error->file
							  + ":"
							  + std::to_string(xml_error->line)
							  + "]; ";
		errors->push_back(error_txt);
	}
}

int get_operator_conf(xmlDocPtr xml_doc, operator_conf & conf)
{
	static const char root_node_name[] = "Operator";
	static const char log_node_name[] = "Log";
	static const char mng_node_name[] = "Manager";

	xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
	if(0 == strcmp((const char*)root_node->name, root_node_name))
	{
		for(xmlNodePtr p = root_node->children; p != NULL; p = p->next)
		{
			if(p->name == NULL)
			{
				continue;
			}

			if(0 == strcmp((const char*)p->name, log_node_name))
			{
				conf.log_conf = new logger_global_conf;
				get_log_global_conf(p, *conf.log_conf);
			}
			else if(0 == strcmp((const char*)p->name, mng_node_name))
			{
				conf.mngr_conf = new manager_conf;
				get_manager_conf(p, *conf.mngr_conf);
			}
		}
	}
	return (NULL != conf.log_conf && NULL != conf.mngr_conf)? 0: -1;
}

int get_log_global_conf(xmlNodePtr log_node, logger_global_conf & conf)
{
	static const char category_node_name[] = "Category";
	static const char level_node_name[] = "Level";
	static const char file_node_name[] = "LogFileName";
	static const char path_node_name[] = "LogFilePath";
	static const char size_node_name[] = "LogFileMaxSizeMB";
	static const char files_node_name[] = "MaxLogFiles";
	static const char layout_node_name[] = "Layout";

	static const u_int8_t CAT_FL = 0x01;
	static const u_int8_t LEV_FL = 0x02;
	static const u_int8_t FIL_FL = 0x04;
	static const u_int8_t DIR_FL = 0x08;
	static const u_int8_t SIZ_FL = 0x10;
	static const u_int8_t FLS_FL = 0x20;
	static const u_int8_t LAY_FL = 0x40;

	u_int8_t set_flags = 0;

	for(xmlNodePtr p = log_node->children; p != NULL; p = p->next)
	{
		if(p->name == NULL)
		{
			continue;
		}

		if(0 == strcmp((const char*)p->name, category_node_name))
		{
			conf.category = (char *)p->children->content;
			set_flags |= CAT_FL;
		}
		else if(0 == strcmp((const char*)p->name, level_node_name))
		{
			conf.level = load_log_level((char *)p->children->content);
			if(800 != conf.level) set_flags |= LEV_FL;
		}
		else if(0 == strcmp((const char*)p->name, file_node_name))
		{
			conf.file = (char *)p->children->content;
			set_flags |= FIL_FL;
		}
		else if(0 == strcmp((const char*)p->name, path_node_name))
		{
			conf.directory = (char *)p->children->content;
			set_flags |= DIR_FL;
		}
		else if(0 == strcmp((const char*)p->name, size_node_name))
		{
			conf.max_size = 1024 * 1024 * strtol((char *)p->children->content, NULL, 10);
			set_flags |= SIZ_FL;
		}
		else if(0 == strcmp((const char*)p->name, files_node_name))
		{
			conf.max_files = strtol((char *)p->children->content, NULL, 10);
			set_flags |= FLS_FL;
		}
		else if(0 == strcmp((const char*)p->name, layout_node_name))
		{
			conf.layout = (char *)p->children->content;
			set_flags |= LAY_FL;
		}
	}
	return (CAT_FL|LEV_FL|FIL_FL|DIR_FL|SIZ_FL|FLS_FL|LAY_FL == set_flags)? 0: -1;
}

int get_manager_conf(xmlNodePtr manager_node, manager_conf & conf)
{
	static const char log_node_name[] = "Log";
	static const char dreamer_node_name[] = "Dreamer";
	static const char executor_node_name[] = "Executor";

	for(xmlNodePtr p = manager_node->children; p != NULL; p = p->next)
	{
		if(p->name == NULL)
		{
			continue;
		}

		if(0 == strcmp((const char*)p->name, log_node_name))
		{
			conf.log_conf = new logger_base_conf;
			get_log_basic_conf(p, *conf.log_conf);
		}
		else if(0 == strcmp((const char*)p->name, dreamer_node_name))
		{
			conf.info_conf = new dreamer_conf;
			get_dreamer_conf(p, *((dreamer_conf *)conf.info_conf));
		}
		else if(0 == strcmp((const char*)p->name, executor_node_name))
		{
			conf.exec_conf = new executor_conf;
			get_executor_conf(p, *conf.exec_conf);
		}
	}
}

int load_log_level(const char * level_txt)
{
	static const char debug_log_level[] = "debug";
	static const char info_log_level[] = "info";
	static const char notice_log_level[] = "notice";
	static const char warning_log_level[] = "warning";
	static const char error_log_level[] = "error";
	static const char critical_log_level[] = "critical";
	static const char alert_log_level[] = "alert";
	static const char fatal_log_level[] = "fatal";

	if(0 == strcmp(debug_log_level, level_txt))
		return 700;
	else if(0 == strcmp(info_log_level, level_txt))
		return 600;
	else if(0 == strcmp(notice_log_level, level_txt))
		return 500;
	else if(0 == strcmp(warning_log_level, level_txt))
		return 400;
	else if(0 == strcmp(error_log_level, level_txt))
		return 300;
	else if(0 == strcmp(critical_log_level, level_txt))
		return 200;
	else if(0 == strcmp(alert_log_level, level_txt))
		return 100;
	else if(0 == strcmp(fatal_log_level, level_txt))
		return 0;
	return 800;
}

int get_log_basic_conf(xmlNodePtr log_node, logger_base_conf & conf)
{
	static const char category_node_name[] = "Category";
	static const char level_node_name[] = "Level";

	static const u_int8_t CAT_FL = 0x01;
	static const u_int8_t LEV_FL = 0x02;

	u_int8_t set_flags = 0;

	for(xmlNodePtr p = log_node->children; p != NULL; p = p->next)
	{
		if(p->name == NULL)
		{
			continue;
		}

		if(0 == strcmp((const char*)p->name, category_node_name))
		{
			conf.category = (char *)p->children->content;
			set_flags |= CAT_FL;
		}
		else if(0 == strcmp((const char*)p->name, level_node_name))
		{
			conf.level = load_log_level((char *)p->children->content);
			if(800 != conf.level) set_flags |= LEV_FL;
		}
	}
	return (CAT_FL|LEV_FL == set_flags)? 0: -1;
}

int get_dreamer_conf(xmlNodePtr dreamer_node, dreamer_conf & conf)
{
	static const char log_node_name[] = "Log";

	for(xmlNodePtr p = dreamer_node->children; p != NULL; p = p->next)
	{
		if(p->name == NULL)
		{
			continue;
		}

		if(0 == strcmp((const char*)p->name, log_node_name))
		{
			conf.log_conf = new logger_base_conf;
			get_log_basic_conf(p, *conf.log_conf);
		}
	}
}

int get_executor_conf(xmlNodePtr executor_node, executor_conf & conf)
{
	static const char log_node_name[] = "Log";

	for(xmlNodePtr p = executor_node->children; p != NULL; p = p->next)
	{
		if(p->name == NULL)
		{
			continue;
		}

		if(0 == strcmp((const char*)p->name, log_node_name))
		{
			conf.log_conf = new logger_base_conf;
			get_log_basic_conf(p, *conf.log_conf);
		}
	}
}

