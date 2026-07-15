#include <slim/common/log.h>
#include <slim/common/utilities.h>
#include <slim/common/validators.h>
namespace slim::common::validators {
	using namespace slim;
	using namespace slim::common;
}
bool slim::common::validators::is_url(const std::string& _string) {
	log::trace(log::Message("slim::common::validators::is_url(_string)","begins => " + _string,__FILE__, __LINE__));
	bool __is_url = false;
	if(_string.length() == 0) {
		log::debug(log::Message("slim::common::validators::is_url(_string)","empty string input found",__FILE__, __LINE__));
	}
	else {
		log::debug(log::Message("slim::common::validators::is_url(_string)","looking for url in string => " + _string,__FILE__, __LINE__));
		std::regex url_regex("(file|https|http)://.+");
		std::smatch url_match_result;
		if(std::regex_match(_string, url_match_result, url_regex)) {
			log::debug(log::Message("slim::common::validators::is_url(_string)",_string + " is url => " + _string,__FILE__, __LINE__));
			__is_url = true;
		}
	}
	log::trace(log::Message("slim::common::validators::is_url()","ends => " + _string + " is_url => " + utilities::to_string(__is_url),__FILE__, __LINE__));
	return __is_url;
}
bool slim::common::validators::is_file_url(const std::string& _string) {
	log::trace(log::Message("slim::common::validators::is_file_url(_string)","begins => " + _string,__FILE__, __LINE__));
	bool __is_file_url = false;
	if(is_url(_string) && _string.starts_with("file://")) {
		log::debug(log::Message("slim::common::validators::is_file_url(_string)",_string + " is a file url => " + _string,__FILE__, __LINE__));
		__is_file_url = true;
	}
	log::trace(log::Message("slim::common::validators::is_file_url(_string)","ends => " + _string + " is_url => " + utilities::to_string(__is_file_url),__FILE__, __LINE__));
	return __is_file_url;
}