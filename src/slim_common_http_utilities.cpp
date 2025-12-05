#include <regex>
#include <string>
#include <slim/common/http/utilities.h>
#include <slim/common/log.h>
namespace slim::common::http::utilities {
	using namespace slim::common;
	std::regex url_pattern("http://|https://|file://");
	std::regex full_url_pattern("((http|https)://)[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
}
std::string slim::common::http::utilities::normalize_url_string(std::string& url_string) {
	log::trace(log::Message("slim::common::http::utilities::normalize_url_string()", "begins url string => " + url_string, __FILE__,__LINE__));
	std::string normalized_url_string = std::regex_search(url_string, url_pattern, std::regex_constants::match_continuous) == 0 ? "http://" + url_string : url_string;
	log::trace(log::Message("slim::common::http::utilities::normalize_url_string()", "ends url string => " + normalized_url_string, __FILE__,__LINE__));
	return normalized_url_string;
}
