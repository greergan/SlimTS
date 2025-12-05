#ifndef __SLIM__COMMON__HTTP__PARSER__H
#define __SLIM__COMMON__HTTP__PARSER__H
#include <string>
#include <slim/common/http/request.h>
#include <slim/common/web_file.h>
namespace slim::common::http::parser {
	void parse_http_request(const char* request_char, slim::common::http::Request* request);
	void parse_http_request(std::string& request_string, slim::common::http::Request* request);
	void parse_http_response(std::stringstream& header_string_stream, Response* response);
}
#endif