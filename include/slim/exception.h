#pragma once
#include <exception>
#include <stdexcept>
#include <string>
namespace slim {
	class SlimException : public std::runtime_error {
		public:
			const std::string call;
			const std::string message;
			const int code;
			SlimException(const std::string& message): std::runtime_error(message), message(message), code(0) {}
			SlimException(const std::string& call, const std::string& message, int code): std::runtime_error(message), call(call), message(message), code(code) {}
	};
}
