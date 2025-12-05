#include <sstream>
#include <string>
#include <slim/common/http/headers.h>
#include <slim/common/http/parser.h>
#include <slim/common/http/request.h>
#include <slim/common/network/address/set.h>
slim::common::http::Request::Request() {}
slim::common::http::Request::Request(char* request_char) {
	slim::common::http::parser::parse_http_request(request_char, this);
}
slim::common::http::Request::Request(std::string& request_string) {
	slim::common::http::parser::parse_http_request(request_string.c_str(), this);
}
const slim::common::network::address::AddressSet& slim::common::http::Request::address_set() const {
	return address_set_value;
}
void slim::common::http::Request::address_set(slim::common::network::address::AddressSet&& address_set_value_in) {
	address_set_value = address_set_value_in;
}
const std::string& slim::common::http::Request::error() const {
	return error_string;
}
void slim::common::http::Request::error(const std::string& value_string) {
	error_string = value_string;
}
slim::common::http::Headers& slim::common::http::Request::headers() {
	return headers_map;
}
const std::string& slim::common::http::Request::method() const {
	return method_string;
}
void slim::common::http::Request::method(const std::string& value_string) {
	method_string = value_string;
}
const std::string& slim::common::http::Request::path() const {
	return path_string;
}
void slim::common::http::Request::path(const std::string& value_string) {
	path_string = value_string;
}
const std::string& slim::common::http::Request::protocol() const {
	return protocol_string;
}
void slim::common::http::Request::protocol(const std::string& value_string) {
	protocol_string = value_string;
}
std::string slim::common::http::Request::to_string() {
	auto space = static_cast<char>(32);
	std::stringstream request_string_stream;
	request_string_stream << method_string << space << path_string << space << version_string << "\r\n";
	for(auto& [key,value] : headers_map.get()) {
		request_string_stream << key << ":" << value << "\r\n";
	}
	request_string_stream << "\r\n\r\n";
	return request_string_stream.str();
}
const std::string& slim::common::http::Request::url() const {
	return url_string;
}
void slim::common::http::Request::url(const std::string& value_string) {
	url_string = value_string;
}
const std::string& slim::common::http::Request::version() const {
	return version_string;
}
void slim::common::http::Request::version(const std::string& value_string) {
	version_string = value_string;
}
