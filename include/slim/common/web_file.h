#ifndef __SLIM__COMMON__WEB__FILE__H
#define __SLIM__COMMON__WEB__FILE__H
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <slim/common/http/headers.h>
#include <slim/common/http/request.h>
#include <slim/common/http/response.h>
#include <slim/common/vector_buffer.h>
namespace slim::common {
	struct WebFile {
		WebFile(slim::common::http::Request&& request_object);
		std::shared_ptr<std::vector<uint8_t>> data();
		const long long& body_offset();
		const int& error_number();
		void error_number(int&& errno_int);
		const std::string& error() const;
		void error(std::string error_string);
		const bool& has_error();
		slim::common::http::Request& request();
		slim::common::http::Response& response();
		std::string to_string();
		private:
			bool errored = false;
			int error_number_value;
			long long body_offset_value;
			std::string error_string_value;
			std::shared_ptr<std::vector<uint8_t>> data_uint8_t_vector_pointer;
			slim::common::http::Request request_object;
			slim::common::http::Response response_object;
	};
}
#endif