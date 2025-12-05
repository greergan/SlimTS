#include <string>
#include <slim/common/http/headers.h>
#include <slim/common/http/parser.h>
#include <slim/common/http/response.h>
slim::common::http::Response::Response() {
	headers_map = Headers();
}
const std::string& slim::common::http::Response::body() {
	return body_string_value;
}
void slim::common::http::Response::body(const std::string& body_string) {
	body_string_value = body_string;	
}
void slim::common::http::Response::body(const char* body_char_pointer) {
	body_string_value = std::string(body_char_pointer);
}
const int& slim::common::http::Response::response_code() const {
	return response_code_int_value;
}
void slim::common::http::Response::response_code(int response_code_int) {
	response_code_int_value = response_code_int;
}
const std::string& slim::common::http::Response::response_code_text() {
	return response_code_string_value;
}
void slim::common::http::Response::response_code_text(std::string response_code_text_string) {
	response_code_string_value = response_code_text_string;
}
void slim::common::http::Response::response_code_text(const char* response_code_text_char_pointer) {
	response_code_string_value = std::string(response_code_text_char_pointer);
}
const std::string& slim::common::http::Response::version() {
	return version_string_value;
}
void slim::common::http::Response::version(std::string version_string) {
	version_string_value = version_string;
}