#include <format>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_set>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/network/address_set.h>
namespace slim::common::http::parser {
	using namespace slim::common;
	void parse_request_line_1(const std::string& method_or_url_string, slim::common::http::Request* _request);
}
void slim::common::http::parser::parse_request_line_1(const std::string& _method_or_url_string, Request* _request) {
	log::trace(log::Message(__func__,"begins",__FILE__,__LINE__));
	log::debug(log::Message(__func__,"URL or method string => " + _method_or_url_string,__FILE__,__LINE__));

	// case remote client request
	if(_method_or_url_string.contains(" ") < 0) {
		std::stringstream method_string_stream(_method_or_url_string);
		std::string value;
		int word_count = 0;
/* 		while(getline(method_string_stream, value, ' ')) {
			switch(line_count) {
				case 0: _request->method(value); break;
				case 1: _request->path(value); break;;
				case 2: _request->version(value); break;
			}
			line_count++;
		} */
		while(method_string_stream >> value) {
			switch(word_count) {
				case 0: _request->method(value); break;
				case 1: _request->path(value); break;;
				case 2: _request->version(value); break;
			}
			word_count++;
		}

	}
	else { // case create valid request object used in remote fetches
		log::debug(log::Message(__func__,"setting request URL => " + _method_or_url_string,__FILE__,__LINE__));
		const URL url = URL(_method_or_url_string);

		_request->protocol(url.protocol().value());
		log::debug(log::Message(__func__,"set request protocol => " + _request->protocol(),__FILE__,__LINE__));

		_request->path(url.pathname().value());
		log::debug(log::Message(__func__,"set request path => " + _request->path(),__FILE__,__LINE__));

		_request->url(_method_or_url_string);
		log::debug(log::Message(__func__,"set request url => " + _request->url(),__FILE__,__LINE__));

		log::debug(log::Message(__func__,"creating AddressSet for remote request => " + url.host().value(),__FILE__,__LINE__));
		//_request->address_set(AddressSet());
		
		if(url.hostname().has_value()) {
			if(url.port().has_value()) {
				_request->headers().set("Host", std::format("{}:{}", url.host().value(), url.port().value()));
			}
			else {
				_request->headers().set("Host", url.host().value());
				log::debug(log::Message(__func__,"set request Host header with hostname only => " + _request->url(),__FILE__,__LINE__));
			}
		}
		

		_request->method("GET");
		_request->version("HTTP/1.1");
		_request->headers().set("Connection", "close"); // move somewhere else
	}
	log::trace(log::Message(__func__,"ends",__FILE__,__LINE__));
}
void slim::common::http::parser::parse_http_request(const char* _string, Request* _request) {
	log::trace(log::Message(__func__,"begins",__FILE__,__LINE__));
	log::debug(log::Message(__func__,"request => " + std::string(_string),__FILE__,__LINE__));
	std::stringstream request_stream(_string);
	//request_stream << _string;
	std::string line;
	int line_number = 1;
	while(getline(request_stream, line)) {
		if(line_number == 1) {
			auto endl_pos = line.find("\r");
			line = line.substr(0, endl_pos);
			parse_request_line_1(line, _request);
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
			_request->headers().set(key, value);
		}
    }
	log::debug(log::Message(__func__,"string => " + _request->to_string(),__FILE__,__LINE__));
	log::trace(log::Message(__func__,"ends",__FILE__,__LINE__));
}
void slim::common::http::parser::parse_http_request(const std::string& _string, Request* _request) {
	log::trace(log::Message("slim::common::http::parser::parse_http_request(_string, _request)","begins => " + _string,__FILE__,__LINE__));
	parse_http_request(_string.c_str(), _request);
	log::trace(log::Message("slim::common::http::parser::parse_http_request(_string, _request)","ends => " + _string,__FILE__,__LINE__));
}




void slim::common::http::parser::parse_http_response(std::stringstream& _header_string_stream, Response* _response) {
	log::trace(log::Message("slim::common::http::parser::parse_http_response(_header_string_stream, _response)","begins",__FILE__,__LINE__));
	bool is_first_line = true;
	std::string line_string;
	while(getline(_header_string_stream, line_string)) {
		if(!is_first_line) {
			if(line_string.empty() || line_string == "\r") {
				break;
			}
			line_string.replace(line_string.find("\r"), 1, "");
			int token_separator_position = line_string.find_first_of(':');
			_response->headers().set(line_string.substr(0, token_separator_position), line_string.substr(token_separator_position + 1)
				.erase(0, line_string.substr(token_separator_position + 1).find_first_not_of(" ")));
		}
		else {
			const int first_space_position = line_string.find_first_of(' ');
			const int second_space_position = line_string.find_first_of(' ', first_space_position + 1);
			_response->version(line_string.substr(0, first_space_position));
			_response->response_code(std::stoi(line_string.substr(first_space_position + 1, 3)));
			_response->response_code_text(line_string.substr(second_space_position + 1));
			is_first_line = false;
		}
	}
	log::trace(log::Message("slim::common::http::parser::parse_http_response(_header_string_stream, _response)","ends",__FILE__,__LINE__));
}