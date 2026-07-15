#ifndef __SLIM__V8__H
#define __SLIM__V8__H
#include <string>
#include <vector>
#include <v8.h>
namespace slim::v_8 {
	void initialize(std::vector<std::string>& _v8_command_line_arguments);
	v8::Isolate* new_isolate(const std::string& _label);
	void tear_down();
}
#endif