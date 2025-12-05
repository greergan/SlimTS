#ifndef __SLIM__COMMON__HTTP__RESPONSE__H
#define __SLIM__COMMON__HTTP__RESPONSE__H
#include <slim/common/http/headers.h>
namespace slim::common::http {
	struct Response {
		Response();
		Headers& headers();
		void body(const char* body_char_pointer);
		void body(const std::string& body_string);
		const std::string& body();
		const int& response_code() const;
		void response_code(int response_code_int);
		const std::string& response_code_text();
		void response_code_text(const char* response_code_text_char_pointer);
		void response_code_text(std::string response_code_text_string);
		const std::string& version();
		void version(std::string version_string);
		private:
			std::string body_string_value;
			int response_code_int_value;
			std::string response_code_string_value;
			std::string version_string_value;
			Headers headers_map;
	};
}
#endif