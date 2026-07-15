#include <format>
#include <string>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/utilities.h>

using namespace slim::common;
using namespace slim::common::http;

slim::common::http::Request::Request() {
	log::trace(log::Message("slim::common::http::Request::Request()","begins/ends empty constructor",__FILE__, __LINE__));
}
slim::common::http::Request::Request(const char* _string) {
	//slim::common::http::parser::parse_http_request(_string, this);


//Host: httpbin.org
//Connection: close
//Accept: */*
//User-Agent: Mozilla/4.0
//Content-Type: application/json

}
slim::common::http::Request::Request(const std::string& _string) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	log::debug(log::Message(__func__,std::format("parsing URL => {}", _string),__FILE__, __LINE__));
	__url = URL(_string);
	log::debug(log::Message(__func__,std::format("parsed URL => {}", _string),__FILE__, __LINE__));
	if(__url.protocol() == "http" || __url.protocol() == "https") {
		__headers.set("Content-Type", "text/plain; charset=utf-8");
		log::debug(log::Message(__func__,std::format("set default Content-Type => {}", "Content-Type"),__FILE__, __LINE__));
	}
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}

const std::optional<storage_container>& slim::common::http::Request::body() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	if(__body.has_value()) {
		log::debug(log::Message(__func__, std::format("{} => returns => size => {}", __func__, __body.value().size()), __FILE__,__LINE__));
	}
	else {
		log::debug(log::Message(__func__, std::format("{} returns => empty => {}", __func__, "std::optional<std::vector<uint8_t>>"), __FILE__,__LINE__));
	}
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __body;
}

const slim::common::http::URL& slim::common::http::Request::url() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("{} returns => URL object", __func__), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __url;
}

const std::string& slim::common::http::Request::version() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("{} returns => {}",__func__, __version), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __version;
}

const std::string slim::common::http::Request::to_string() {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));

	bool add_body = false;

	std::string string_value = std::format("{} {} {}\r\n", __method, __url.pathname().value(), __version);

	if(__body.has_value() && __method != "GET" && __method != "DELETE") {
		__headers.set("content-length", std::to_string(__body.value().size()));
		add_body = true;
	}

	for(const auto& [key,value] : __headers.get() ) {
		string_value += std::format("{}:{}\r\n", key, value);
		log::debug(log::Message(__func__, std::format("{} => added header => {} => {}", __func__, key, value),__FILE__, __LINE__));
	}

	string_value += "\r\n\r\n";

	if(add_body) {
		string_value += std::format("{}", (char*)__body.value().data());
		log::debug(log::Message(__func__, std::format("{} => added => {} => {} bytes", __func__, "body", __body.value().size()),__FILE__, __LINE__));
	}

	log::debug(log::Message(__func__, std::format("{} => {}", __func__,  string_value),__FILE__, __LINE__));
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
	return string_value;
}

