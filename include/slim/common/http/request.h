#ifndef __SLIM__COMMON__HTTP__REQUEST__H
#define __SLIM__COMMON__HTTP__REQUEST__H
#include <slim/common/network/address/set.h>
#include <slim/common/http/headers.h>
namespace slim::common::http {
	class Request {
		slim::common::network::address::AddressSet address_set_value;
		std::string error_string;
		std::string method_string;
		std::string path_string;
		std::string protocol_string;
		std::string version_string;
		std::string url_string;
		Headers headers_map;
		public:
			Request();
			Request(char* request_pointer);
			Request(std::string& request_string);
			const slim::common::network::address::AddressSet& address_set() const;
			void address_set(slim::common::network::address::AddressSet&& address_set_value_in);
			const std::string& error() const;
			void error(const std::string& value);
			const std::string& path() const;
			void path(const std::string& value);
			const std::string& protocol() const;
			void protocol(const std::string& value);
			const std::string& method() const;
			void method(const std::string& value);
			std::string to_string();
			const std::string& url() const;
			void url(const std::string& value);
			const std::string& version() const;
			void version(const std::string& value);
			slim::common::http::Headers& headers();
	};
};
#endif