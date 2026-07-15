#ifndef __SLIM__COMMAND_LINE_HANDLER__H
#define __SLIM__COMMAND_LINE_HANDLER__H
#include <string>
#include <vector>
namespace slim::command_line {
	std::vector<std::string> parse(int argc, char *argv[]);
	const std::string& get_script_name();
}
#endif