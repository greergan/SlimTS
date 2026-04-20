#ifndef __SLIM__COMMON__LOG__H
#define __SLIM__COMMON__LOG__H
#include <string>
#include <string_view>
#include <unistd.h>
namespace slim::common::log {
	struct Message {
		std::string consumer;
		std::string file;
		std::string function;
		std::string_view label;
		std::string log_level;
		std::string_view separator;
		std::string text;
		const int line;
		pid_t process_id;
		Message() = delete;
		Message(std::string_view _function, std::string_view _text, std::string_view _file,
			const int _line, std::string_view _consumer = "console", std::string_view _separator = "|");
	};

	void error(Message message);
	void debug(Message message);
	void info(std::string_view _string);
	void trace(Message message);
	void print(const Message message);
}
#endif