#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <slim/common/log.h>
namespace slim::common::log {
	std::mutex cerr_mutex;
	std::mutex cout_mutex;
	std::mutex label_mutex;
	std::string field_separator = "|";
	bool allow_color_print = true;
	bool allow_debug_print = false;
	bool allow_error_print = true;
	bool allow_info_print = false;
	bool allow_trace_print = false;
	bool allow_warn_print = false;
	bool allow_thread_identity_print = false;
	std::unordered_map<std::string, std::string> colors {
		{"black",  "\033[0;30m"},
		{"blue",   "\033[0;34m"},
		{"cyan",   "\033[0;36m"},
		{"green",  "\033[0;32m"},
		{"magenta","\033[0;35m"},
		{"red",    "\033[0;31m"},
		{"white",  "\033[0;37m"},
		{"yellow", "\033[0;33m"}
	};
	auto colorize = [](std::string input, std::string color)->std::string {
		return allow_color_print ? std::string(colors[color]) + input + "\033[0m" : input;
	};
	auto black  = [](std::string input)->std::string {return colorize(input, std::string("black"));};
	auto blue   = [](std::string input)->std::string {return colorize(input, std::string("blue"));};
	auto cyan   = [](std::string input)->std::string {return colorize(input, std::string("cyan"));};
	auto green  = [](std::string input)->std::string {return colorize(input, std::string("green"));};
	auto magenta= [](std::string input)->std::string {return colorize(input, std::string("magenta"));};
	auto red    = [](std::string input)->std::string {return colorize(input, std::string("red"));};
	auto yellow = [](std::string input)->std::string {return colorize(input, std::string("yellow"));};
	auto white  = [](std::string input)->std::string {return colorize(input, std::string("white"));};
	auto get_identifiers = []()->std::string {
		std::stringstream identity_stream;
		identity_stream << "[" << getpid() << field_separator << std::this_thread::get_id() << "]";
		return identity_stream.str();
	};
	auto construct_label = [](std::string label_string)->std::string {
		std::string identity_string =
			allow_thread_identity_print ? label_string == "INFO" ? yellow(get_identifiers()) : white(get_identifiers()) : "";
		std::string(*color)(std::string);
		if(label_string == "DEBUG") { color = yellow; }
		else if(label_string == "ERROR") { color = red; }
		else if(label_string == "INFO")  { color = white; }
		else if(label_string == "TRACE") { color = cyan; }
		else if(label_string == "WARN")  { color = magenta; }
		return color(label_string) + identity_string + color("=>");
	};
}
void slim::common::log::print_all(bool value) {
	allow_debug_print = value;
	allow_error_print = value;
	allow_info_print = value;
	allow_trace_print = value;
	allow_warn_print = value;
	allow_thread_identity_print = value;
}
void slim::common::log::print_color(bool value) {allow_color_print = value;}
void slim::common::log::print_debug(bool value) {allow_debug_print = value;}
void slim::common::log::print_error(bool value) {allow_error_print = value;}
void slim::common::log::print_info(bool value) {allow_info_print = value;}
void slim::common::log::print_trace(bool value) {allow_trace_print = value;}
void slim::common::log::print_thread_identity(bool value) {allow_thread_identity_print = value;}
void slim::common::log::print_warn(bool value) {allow_warn_print = value;}
void slim::common::log::debug(Message message) {
	if(allow_debug_print) {
		print(construct_label("DEBUG"), message);
	}
}
void slim::common::log::error(Message message) {
	if(allow_error_print) {
		print(construct_label("ERROR"), message);
	}
}
void slim::common::log::info(Message message) {
	if(allow_info_print) {
		print(construct_label("INFO"), message);
	}
}
void slim::common::log::trace(Message message) {
	if(allow_trace_print) {
		print(construct_label("TRACE"), message);
	}
}
void slim::common::log::warn(Message message) {
	if(allow_warn_print) {
		print(construct_label("WARN"), message);
	}
}
void slim::common::log::info(character_types value) {
	std::lock_guard<std::mutex> lock(cout_mutex);
	auto value_t = std::holds_alternative<const char*>(value) ? std::get<const char*>(value) : std::get<std::string>(value);
	std::cout << value_t << "\n";
}
void slim::common::log::print(std::string label, Message message, bool is_error) {
/* 	if(!message.call.starts_with("slim::package::http")
		&& !message.call.starts_with("slim::common::WebFile")
		&& !message.call.starts_with("slim::common::fetch::web_file")
		&& !message.call.starts_with("slim::common::network::client::tcp")
		&& !message.call.starts_with("slim::common::http::parser::parse_http_response")) return; */
	std::stringstream print_text;
	int width = allow_color_print ? 16 : 4;
	print_text << label << std::setw(width) << magenta(std::to_string(message.line_number)) << field_separator
		<< magenta(message.file) << field_separator << green(message.call) << field_separator << white(message.text) << "\n";
	if(is_error) {
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << print_text.str();
	}
	else {
		std::lock_guard<std::mutex> lock(cout_mutex);
		std::cout << print_text.str();
	}
}
slim::common::log::Message::Message(character_types call, character_types text, character_types file, int line_number) : line_number(line_number) {
	this->call = std::holds_alternative<const char*>(call) ? std::get<const char*>(call)
		: std::holds_alternative<char*>(call) ? std::string(std::get<char*>(call)) : std::get<std::string>(call);
	this->text = std::holds_alternative<const char*>(text) ? std::string(std::get<const char*>(text))
		: std::holds_alternative<char*>(text) ? std::string(std::get<char*>(text)) : std::get<std::string>(text);
	this->file = std::holds_alternative<const char*>(file) ? std::string(std::get<const char*>(file))
		: std::holds_alternative<char*>(file) ? std::string(std::get<char*>(file)) : std::get<std::string>(file);
}