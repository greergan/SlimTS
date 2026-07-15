#include <format>
#include <string>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/utilities.h>


slim::common::http::Headers::Headers() {}

const slim::common::http::header_map& slim::common::http::Headers::get() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("{} => returns => size => {}", __func__, __headers.size()), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __headers;
}

[[nodiscard]]
const bool slim::common::http::Headers::set(const std::string& _key, const std::string& _value) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));

	const auto key = slim::common::utilities::to_lower(_key);
	const auto pair = __headers.emplace(_key, _value);

	if(pair.second) {
		log::debug(log::Message(__func__, std::format("{} => {} => {} => successful", __func__, _key, _value), __FILE__,__LINE__));
	}
	else {
		log::debug(log::Message(__func__, std::format("{} => {} => {} => not successful", __func__, key, _value), __FILE__,__LINE__));
		log::error(log::Message(__func__, std::format("{} => {} => {} => not successful", __func__, key, _value), __FILE__,__LINE__));
	}
	
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return pair.second;
}
