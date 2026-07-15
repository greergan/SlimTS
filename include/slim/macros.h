#ifndef __SLIM__MACROS__H
#define __SLIM__MACROS__H
#include <memory>
#include <string>
#include <sstream>
namespace slim::macros {
	std::unique_ptr<std::string> apply(std::shared_ptr<std::string> _content_pointer, const std::string& _specifier_string);
	std::unique_ptr<std::stringstream> apply(std::unique_ptr<std::stringstream> _input_stringstream_pointer, const std::string& _absolute_file_path_string);
}
#endif