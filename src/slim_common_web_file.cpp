#include <algorithm>
#include <format>
#include <memory>
#include <sstream>
#include <vector>
#include <slim/common/fetch.h>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/web_file.h>

slim::common::WebFile::WebFile() {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	__data_ptr = std::make_shared<std::vector<uint8_t>>();
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}
slim::common::WebFile::WebFile(const std::string _string) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	__data_ptr = std::make_shared<std::vector<uint8_t>>();
	log::debug(log::Message(__func__,"parsing request => " + _string,__FILE__, __LINE__));
	request(slim::common::http::Request(_string));
	log::debug(log::Message(__func__,"fetching => " + _string,__FILE__, __LINE__));
	slim::common::fetch::web_file(this);
	log::debug(log::Message(__func__,"fetched => " + request().url().url().value_or("not set"),__FILE__, __LINE__));
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
}
slim::common::WebFile::WebFile(http::Request&& _request): __request(_request) {
	log::trace(log::Message("slim::common::WebFile::WebFile(request_object)","begins => " + _request.url().url().value_or("not set"),__FILE__, __LINE__));
	__data_ptr = std::make_shared<std::vector<uint8_t>>();
	slim::common::fetch::web_file(this);
	log::debug(log::Message("slim::common::WebFile::WebFile()","fetched => " + __request.url().url().value_or("not set"),__FILE__, __LINE__));
	std::stringstream header_stream;
	uint8_t endof_headers[4] = {'\0','\0','\0','\0'};
	for(auto& c : *__data_ptr) {
		endof_headers[0] = endof_headers[1];
		endof_headers[1] = endof_headers[2];
		endof_headers[2] = endof_headers[3];
		endof_headers[3] = c;
		if(endof_headers[0] == '\r' && endof_headers[1] == '\n' && endof_headers[2] == '\r' && endof_headers[3] == '\n') {
			header_stream << c;
			body_offset(header_stream.str().size());
			break;
		}
		else {
			header_stream << c;
		}
	}
	__data_ptr.get()->erase(__data_ptr.get()->begin(),__data_ptr.get()->begin() + body_offset());
	response(slim::common::http::Response());

//slim::common::http::parser::parse_http_response(header_stream, &response());

	log::trace(log::Message("slim::common::WebFile::WebFile()","ends => " + __request.url().url().value_or("not set"),__FILE__, __LINE__));
}
void slim::common::WebFile::body_offset(const long long& _long_long) {
	log::trace(log::Message("slim::common::WebFile::body_offset(_long_long)","sets => " + std::to_string(_long_long),__FILE__, __LINE__));
	__body_offset = _long_long;
}
const long long& slim::common::WebFile::body_offset() const {
	log::trace(log::Message("slim::common::WebFile::body_offset()","returns => " + std::to_string(__body_offset),__FILE__, __LINE__));
	return __body_offset;
}
std::shared_ptr<std::vector<uint8_t>> slim::common::WebFile::data() {
	log::trace(log::Message("slim::common::WebFile::data()","returns => __data_ptr",__FILE__, __LINE__));
	//log::debug(log::Message("slim::common::WebFile::data()","__data_ptr => " + std::string((char*)__data_ptr.get()->data()),__FILE__, __LINE__));
	return __data_ptr;
}
std::shared_ptr<std::vector<uint8_t>> slim::common::WebFile::response_data() {
	log::trace(log::Message(__func__,"returns => __response_data_ptr",__FILE__, __LINE__));
	return __response_data_ptr;
}
const int& slim::common::WebFile::error_number() const {
	log::trace(log::Message("slim::common::WebFile::error_number()","returns => " + std::to_string(__error_number),__FILE__, __LINE__));
	return __error_number;
}
void slim::common::WebFile::error_number(int _int) {
	log::trace(log::Message("slim::common::WebFile::error_number(_int)","sets => " + std::to_string(_int),__FILE__, __LINE__));
	__error_number = _int;
}
const std::string& slim::common::WebFile::error() const {
	log::trace(log::Message("slim::common::WebFile::error_number()","returns => " + __error_string,__FILE__, __LINE__));
	return __error_string;
}
void slim::common::WebFile::error(const std::string _string) {
	log::trace(log::Message("slim::common::WebFile::error_number(_string)","sets => " + _string,__FILE__, __LINE__));
	__error_string = _string;
}
void slim::common::WebFile::request(slim::common::http::Request _request) {
	log::trace(log::Message("slim::common::WebFile::request(_request)","sets => " + _request.url().url().value(),__FILE__, __LINE__));
	__request = _request;
}
slim::common::http::Request& slim::common::WebFile::request() {
	log::trace(log::Message("slim::common::WebFile::request()","returns => " + __request.url().url().value(),__FILE__, __LINE__));
	return __request;
}
void slim::common::WebFile::response(slim::common::http::Response _response) {
	log::trace(log::Message("slim::common::WebFile::response(_response)","sets => _response",__FILE__, __LINE__));
	__response = _response;
}
slim::common::http::Response& slim::common::WebFile::response() {
	log::trace(log::Message("slim::common::WebFile::response()","returns => __response",__FILE__, __LINE__));
	return __response;
}
const long long slim::common::WebFile::size() {
	log::trace(log::Message("slim::common::WebFile::size()","returns => long long ",__FILE__, __LINE__));
	return __data_ptr.get() ? __data_ptr.get()->size() : 0;
}
std::string slim::common::WebFile::to_string() {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("returns => {} ", (char*)__data_ptr.get()->data()),__FILE__, __LINE__));
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
	return std::string((char*)__data_ptr.get()->data());
}
