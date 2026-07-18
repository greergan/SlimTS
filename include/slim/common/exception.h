#ifndef SLIM_COMMON_EXCEPTION_H
#define SLIM_COMMON_EXCEPTION_H
#include <exception>
#include <stdexcept>
#include <string>
namespace slim::common {
	class SlimException : public std::exception  {
		public:
			const std::string call;
			const std::string message;
			const int code;
			SlimException(const char* message): message(std::string(message)), code(0) {}
			SlimException(const char* call, const char* message, int code): call(std::string(call)), message(std::string(message)), code(code) {}
	};
	class SlimFileException : public SlimException  {
		public:
			const std::string path;
			SlimFileException(const char* message): SlimException(message) {}
			SlimFileException(const char* call, const char* message, const char* path, int code): path(std::string(path)), SlimException(call, message, code){}
	};
}
#endif
