#include <filesystem>
#include <sstream>
#include <string>
#include <slim/common/http/headers.h>
#include <slim/common/http/parser.h>
#include <slim/common/http/utilities.h>
#include <slim/common/network/address/set.h>
#include <slim/common/log.h>
namespace slim::common::http::parser {
	using namespace slim::common;
	void parse_request_line_1(std::string& method_or_url_string, slim::common::http::Request* request);
}
void slim::common::http::parser::parse_request_line_1(std::string& method_or_url_string, Request* request) {
	log::trace(log::Message("slim::common::http::parser::parse_request_line_1()","begins",__FILE__,__LINE__));
	log::debug(log::Message("slim::common::http::parser::parse_request_line_1()","data => " + method_or_url_string,__FILE__,__LINE__));
	// case remote client request
	if(method_or_url_string.contains(" ") < 0) {
		std::stringstream method_string_stream(method_or_url_string);
		std::string value;
		int line_count = 0;
		while(getline(method_string_stream, value, ' ')) {
			switch(line_count) {
				case 0: request->method(value); break;
				case 1: request->path(value); break;;
				case 2: request->version(value); break;
			}
			line_count++;
		}
	}
	else { // case create valid request object used in fetches
		std::string url_string = http::utilities::normalize_url_string(method_or_url_string);
		request->url(url_string);
		int pop_front_count = url_string.starts_with("http://") || url_string.starts_with("file://") ? 7 : 8; //https://
		std::string host_or_path_string = url_string.substr(pop_front_count);
		request->protocol(url_string.substr(0, pop_front_count));
		if(request->protocol() == "file://") {
			host_or_path_string.starts_with("/") ? request->path(host_or_path_string) : request->path(std::filesystem::absolute(host_or_path_string).string());
		}
		else {
			int first_slash_position = host_or_path_string.find_first_of("/");
			if(first_slash_position == 0) {
				log::error(log::Message("slim::common::http::parser::parse_line_1()","Host not found in URL => " + method_or_url_string,__FILE__,__LINE__));
				request->error("Host not found in URL => " + method_or_url_string);
			}
			else {
				std::string temp_host_port_string;
				if(first_slash_position > 0) {
					temp_host_port_string = host_or_path_string.substr(0, first_slash_position);
					request->path(host_or_path_string.substr(first_slash_position));
				}
				else {
					temp_host_port_string = host_or_path_string;
					request->path("/");
				}
				if(!host_or_path_string.contains(":")) {
					temp_host_port_string += request->protocol() == "http://" ? ":80" : ":443";
				}
				request->address_set(slim::common::network::address::string_to_address_set(temp_host_port_string));
				request->headers().set("Host", request->address_set().address);
			}
		}
		request->method("GET");
		request->version("HTTP/1.1");
		request->headers().set("Connection", "close"); // move somewhere else
	}
	log::trace(log::Message("slim::common::http::parser::parse_line_1()","ends url path => " + request->url(),__FILE__,__LINE__));
}
void slim::common::http::parser::parse_http_request(const char* request_char, Request* request) {
	log::trace(log::Message("slim::common::http::parser::parse_http_request()","begins",__FILE__,__LINE__));
	log::debug(log::Message("slim::common::http::parser::parse_http_request()",request_char,__FILE__,__LINE__));
	std::stringstream request_stream;
	request_stream << request_char;
	std::string line;
	int line_number = 1;
	while(getline(request_stream, line)) {
		if(line_number == 1) {
			auto endl_pos = line.find("\r");
			line = line.substr(0, endl_pos);
			parse_request_line_1(line, request);
			line_number++;
		}
		else if(line.find("\r") == 0) {
			break;
		}
		else if(line.find(" ")) {
			auto endl_pos = line.find("\r");
			line = line.substr(0, endl_pos);
			int token_position = line.find(" ");
			std::string key = line.substr(0, token_position - 1);
			std::string value = line.substr(token_position + 1);
			request->headers().set(key, value);
		}
    }
	log::trace(log::Message("slim::common::http::parser::parse_http_request()","ends url path => " + request->path(),__FILE__,__LINE__));
}
void slim::common::http::parser::parse_http_request(std::string& request_string, Request* request) {
	parse_http_request(request_string.c_str(), request);
}
void slim::common::http::parser::parse_http_response(std::stringstream& header_string_stream, Response* response) {
	log::trace(log::Message("slim::common::http::parser::parse_http_response()","begins",__FILE__,__LINE__));
	bool is_first_line = true;
	std::string line_string;
	while(getline(header_string_stream, line_string)) {
		if(!is_first_line) {
			if(line_string.empty() || line_string == "\r") {
				break;;
			}
			line_string.replace(line_string.find("\r"), 1, "");
			int token_separator_position = line_string.find_first_of(':');
			response->headers().set(line_string.substr(0, token_separator_position), line_string.substr(token_separator_position + 1)
				.erase(0, line_string.substr(token_separator_position + 1).find_first_not_of(" ")));
		}
		else {
			int first_space_position = line_string.find_first_of(' ');
			int second_space_position = line_string.find_first_of(' ', first_space_position + 1);
			response->version(line_string.substr(0, first_space_position));
			response->response_code(std::stoi(line_string.substr(first_space_position + 1, 3)));
			response->response_code_text(line_string.substr(second_space_position + 1));
			is_first_line = false;
		}
	}
	

	log::trace(log::Message("slim::common::http::parser::parse_http_response()","ends",__FILE__,__LINE__));
}