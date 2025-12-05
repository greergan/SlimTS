#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>
#include <slim/common/fetch.h>
#include <slim/common/http/parser.h>
#include <slim/common/http/request.h>
#include <slim/common/http/response.h>
#include <slim/common/log.h>
#include <slim/common/vector_buffer.h>
#include <slim/common/web_file.h>

#include <iostream>

slim::common::WebFile::WebFile(http::Request&& request_object) : request_object(request_object) {
	log::trace(log::Message("slim::common::WebFile::WebFile()","begins url => " + request_object.url(),__FILE__, __LINE__));
	data_uint8_t_vector_pointer = std::make_shared<std::vector<uint8_t>>();
	slim::common::fetch::web_file(this);
	log::debug(log::Message("slim::common::WebFile::WebFile()","fetched url => " + request_object.url(),__FILE__, __LINE__));
	std::stringstream header_stream;
	uint8_t endof_headers[4] = {'\0','\0','\0','\0'};
	for(auto& c : *data_uint8_t_vector_pointer) {
		endof_headers[0] = endof_headers[1];
		endof_headers[1] = endof_headers[2];
		endof_headers[2] = endof_headers[3];
		endof_headers[3] = c;

		if(endof_headers[0] == '\r' && endof_headers[1] == '\n' && endof_headers[2] == '\r' && endof_headers[3] == '\n') {
			header_stream << c;
			body_offset_value = header_stream.str().size();
			break;
		}
		else {
			header_stream << c;
		}
	}
	data_uint8_t_vector_pointer.get()->erase(data_uint8_t_vector_pointer.get()->begin(),data_uint8_t_vector_pointer.get()->begin() + body_offset_value);
	log::debug(log::Message("slim::common::WebFile::WebFile()","fetched url => " + request_object.url(),__FILE__, __LINE__));
	//log::info(header_stream.str());
	response_object = slim::common::http::Response();
	slim::common::http::parser::parse_http_response(header_stream, &response_object);
	//slim::common::http::parser::parse_http_response(this);
	log::trace(log::Message("slim::common::WebFile::WebFile()","ends url => " + request_object.url(),__FILE__, __LINE__));
}
const long long& slim::common::WebFile::body_offset() {
	return body_offset_value;
}
std::shared_ptr<std::vector<uint8_t>> slim::common::WebFile::data() {
	return data_uint8_t_vector_pointer;
}
const bool& slim::common::WebFile::has_error() {
	return errored;
}
const int& slim::common::WebFile::error_number() {
	return error_number_value;
}
void slim::common::WebFile::error_number(int&& error_int) {
	error_number_value = error_int;
	errored = true;
}
const std::string& slim::common::WebFile::error() const {
	return error_string_value;
}
void slim::common::WebFile::error(std::string error_string) {
	error_string_value = error_string;
	errored = true;
}
slim::common::http::Request& slim::common::WebFile::request() {
	return request_object;
}
slim::common::http::Response& slim::common::WebFile::response() {
	return response_object;
}
std::string slim::common::WebFile::to_string() {
	return std::string((char*)data_uint8_t_vector_pointer.get()->data());
}
