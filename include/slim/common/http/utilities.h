#ifndef __SLIM__COMMON__HTTP__UTILITIES__H
#define __SLIM__COMMON__HTTP__UTILITIES__H
#include <regex>
#include <string>
namespace slim::common::http::utilities {
	std::string normalize_url_string(std::string& url_string);
}
#endif