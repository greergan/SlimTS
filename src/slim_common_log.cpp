#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <string_view>
#include <slim/common/log.h>
#include <slim/configuration_handler.h>
namespace slim::common::log {
	std::mutex cerr_mutex;
	std::mutex cout_mutex;

	std::unordered_map<std::string_view, std::string_view> color_codes {
		{"black",   "\033[0;30m"},
		{"blue",    "\033[0;34m"},
		{"cyan",    "\033[0;36m"},
		{"green",   "\033[0;32m"},
		{"magenta", "\033[0;35m"},
		{"red",     "\033[0;31m"},
		{"white",   "\033[0;37m"},
		{"yellow",  "\033[0;33m"}
	};

	std::unordered_map<std::string_view, std::string_view> colors {
		{"END",     "\033[0m"},
		{"DEBUG",   color_codes["yellow"]},
		{"ERROR",   color_codes["red"]},
		{"FILE",    color_codes["magenta"]},
		{"FUNCTION",color_codes["green"]},
		{"INFO",    color_codes["white"]},
		{"LINE",    color_codes["magenta"]},
		{"TEXT",    color_codes["white"]},
		{"TRACE",   color_codes["cyan"]}
	};
}

void slim::common::log::debug(Message _message) {
	_message.log_level = "debug";

	if(slim::configuration_handler::can_log(_message)) {
		_message.label = "DEBUG";
		print(_message);
	}
}

void slim::common::log::error(Message _message) {
	_message.log_level = "error";
	
	if(slim::configuration_handler::can_log(_message)) {
		_message.label = "ERROR";
		print(_message);
	}
}

void slim::common::log::info(std::string_view _value) {
	std::lock_guard<std::mutex> lock(cout_mutex);
	std::cout << colors["INFO"] << _value << colors["END"] << std::endl;
}

void slim::common::log::trace(Message _message) {
	_message.log_level = "trace";

	if(slim::configuration_handler::can_log(_message)) {
		_message.label = "TRACE";
		print(_message);
	}
}

void slim::common::log::print(const Message _message) {
	std::lock_guard<std::mutex> lock((_message.log_level == "error") ? cerr_mutex : cout_mutex);
	static std::ostream& print_stream = _message.log_level == "error" ? std::cerr : std::cout;

	print_stream << colors[_message.label] << _message.label << "=>" << colors["END"];
	print_stream << std::setw(16) << colors["LINE"] << std::to_string(_message.line) << colors["END"];
	print_stream << _message.separator;
	print_stream << colors["FILE"] << _message.file << colors["END"];
	print_stream << _message.separator;
	print_stream << colors["FUNCTION"] << _message.function << colors["END"];
	print_stream << _message.separator;
	print_stream << colors["TEXT"] << _message.text << colors["END"];
	print_stream << std::endl;
}

slim::common::log::Message::Message(std::string_view _function, std::string_view _text, std::string_view _file,
	const int _line, std::string_view _consumer, std::string_view _separator)
		: consumer(_consumer), function(_function), file(_file), text(_text), line(_line), separator(_separator) {

	process_id = getpid();
}