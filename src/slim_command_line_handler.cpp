#include <filesystem>
#include <regex>
#include <system_error>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <slim/command_line_handler.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/memory/mapper.h>
#include <slim/path.h>
#include <slim/slim.h>

#include <iostream>
#include <cstdlib>

namespace slim::command_line {
	using namespace slim::common;

	std::string script_arguments;
	std::unordered_map<std::string, std::string> slim_configuration_values;
	std::vector<std::string> v8_configuration_values;
	std::vector<std::string> library_paths;
	std::unordered_set<std::string> allowed_file_extensions{"",".js",".mjs",".ts"};
	std::unordered_set<std::string> slim_configurations {"--lib", "--use-cache","--cache-dir","--create-configurations","-d","-v","--version","-w"};
	std::unordered_map<std::string, bool> slim_command_line_argument_expects_value {
		{"--lib",true},{"--cache-dir",true}
	};
	auto is_script = [](std::string& argument)->std::string {
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::is_script()","begins => " + argument, __FILE__,__LINE__));
#endif
		std::string script_name_string;
		if(argument.starts_with("http://") || argument.starts_with("https://")) {
			script_name_string = argument;
			script_arguments += script_name_string + ",";
			slim_configuration_values["script_name"] = script_name_string;
		}
		else {
			auto argument_string = std::regex_replace(argument, std::regex("/./|//"), "/");
			auto script_path = std::filesystem::absolute(argument_string);
			if(script_path.string().length() > 1) {
				if(allowed_file_extensions.contains(script_path.extension().string())) {
					script_name_string = script_path.string();
					script_arguments += script_name_string + ",";
					slim_configuration_values["script_name"] = script_name_string;
				}
			}
		}
		auto answer_message_string = + script_name_string.length() > 1 ? "true" : "false";
#ifdef ENABLE_LOGGING
		log::debug(log::Message("slim::command_line::is_script()",argument + " is_script => " + answer_message_string, __FILE__,__LINE__));
#endif
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::is_script()","ends => " + argument, __FILE__,__LINE__));
#endif
		return script_name_string;
	};
	auto is_slim_argument = [](std::string& argument)->bool {
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::is_slim_argument()","begins => " + argument, __FILE__,__LINE__));
#endif
		auto answer = slim_configurations.contains(argument);
		auto answer_message_string = + answer ? "true" : "false";
#ifdef ENABLE_LOGGING
		log::debug(log::Message("slim::command_line::is_slim_argument()",argument + " is_slim_argument => " + answer_message_string, __FILE__,__LINE__));
#endif
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::is_slim_argument()","ends => " + argument, __FILE__,__LINE__));
#endif
		return answer;
	};
	auto slim_argument_expects_value = [](std::string& argument)->bool {
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::slim_argument_expects_value()","begins => " + argument, __FILE__,__LINE__));
#endif
		auto answer = slim_command_line_argument_expects_value.find(argument) != slim_command_line_argument_expects_value.end()
			&& slim_command_line_argument_expects_value.find(argument)->second;
		auto answer_message_string = + answer ? "true" : "false";
#ifdef ENABLE_LOGGING
		log::debug(log::Message("slim::command_line::slim_argument_expects_value()",argument + " expects value => " + answer_message_string, __FILE__,__LINE__));
#endif
#ifdef ENABLE_LOGGING
		log::trace(log::Message("slim::command_line::slim_argument_expects_value()","ends => " + argument, __FILE__,__LINE__));
#endif
		return answer;
	};
}
std::vector<std::string> slim::command_line::parse(int argc, char *argv[]) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
	slim_configuration_values["slim_executable"] = slim::path::getExecutablePath();
	script_arguments = slim_configuration_values["slim_executable"] + ",";
	bool found_module_source_file = false;
	try {
		for(int index = 1; index < argc; index++) {
			auto argument = std::string(argv[index]);
			if(found_module_source_file) {
#ifdef ENABLE_LOGGING
				log::debug(log::Message(__func__, "script argument => " + argument,__FILE__,__LINE__));
#endif
				script_arguments += argument + ",";
			}
			else if(is_slim_argument(argument)) {
#ifdef ENABLE_LOGGING
				log::debug(log::Message(__func__, "slim argument => " + argument,__FILE__,__LINE__));
#endif
				if(argument == "-d") {
					memory_mapper::write("slim_runtime_environmental_variables", "daemon", true);
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__,"daemon => true",__FILE__,__LINE__));
#endif
				}
				else if(argument == "-v" || argument == "--version") {
					slim::version();
				}
				else if(argument == "-w") {
					memory_mapper::write("slim_runtime_environmental_variables", "watching_files", true);
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__,"watching_files => true",__FILE__,__LINE__));
#endif
				}
				else if(argument == "--use-cache") {
					slim_configuration_values["use_cache"] = "true";
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__,"use_cache => true",__FILE__,__LINE__));
#endif
				}
				else if(argument == "--cache-dir") {
					auto cache_directory_argument = std::filesystem::path(argv[++index]);
					if(!std::filesystem::exists(cache_directory_argument)) {
						std::error_code error_code;
						if(std::filesystem::create_directories(cache_directory_argument, error_code)) {
#ifdef ENABLE_LOGGING
							log::debug(log::Message(__func__,"cache_directory created => " + cache_directory_argument.string(),__FILE__,__LINE__));
#endif
						}
						else {
							const std::string error_string = "could not create cache_directory => " + cache_directory_argument.string() + " => " + error_code.message();
#ifdef ENABLE_LOGGING
							log::error(log::Message(__func__,error_string,__FILE__,__LINE__));
#endif
							throw "slim::command_line::parse()|" + error_string;
						}
					}
					slim_configuration_values["cache_directory"] = std::filesystem::canonical(cache_directory_argument).string();
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__,"cache_directory => " + slim_configuration_values["cache_directory"],__FILE__,__LINE__));
#endif
				}
				else if(argument == "--create-configurations") {
std::cout << "here\n";
				}
				else if(argument == "--lib") {
					auto lib_directory_argument = std::filesystem::path(argv[++index]);
					if(std::filesystem::exists(lib_directory_argument)) {
						auto canonical_directory = std::filesystem::canonical(lib_directory_argument);
						library_paths.push_back(canonical_directory.string());
						auto node_modules_path = std::filesystem::absolute(canonical_directory.string() + std::filesystem::path::preferred_separator + "node_modules");
						if(std::filesystem::exists(node_modules_path)) {
							library_paths.push_back(std::filesystem::canonical(node_modules_path).string());
						}
					}
				}
				else if(slim_argument_expects_value(argument)) {
					slim_configuration_values[argument] = argv[++index];
				}
			}
			else {
				auto script_file = is_script(argument);
				if(!script_file.empty()) {
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__, "script file name found => " + argument,__FILE__,__LINE__));
#endif
					found_module_source_file = true;
				}
				else {
					auto argument_string = std::string(argv[index]);
#ifdef ENABLE_LOGGING
					log::debug(log::Message(__func__, "possible v8 argument found => " + argument_string,__FILE__,__LINE__));
#endif
					v8_configuration_values.push_back(argument_string);
				}
			}
		}
	}
	catch(const std::bad_alloc& bad_alloc_error) {
		const std::string error_message_string = "push_back failed => " + std::string(bad_alloc_error.what());
#ifdef ENABLE_LOGGING
		log::error(log::Message(__func__, error_message_string,__FILE__, __LINE__));
#endif
		throw error_message_string;
	}
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__, "command line has been parsed",__FILE__,__LINE__));
#endif
	if(script_arguments.ends_with(",")) {
		script_arguments.pop_back();
	}
	memory_mapper::write("configurations", "script.argv", script_arguments);
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__, "script arguments have been written",__FILE__,__LINE__));
#endif
	std::string slim_library_path;
	for(auto& slim_library_location : library_paths) {
		if(slim_library_path.length() == 0) {
			slim_library_path = slim_library_location;
		}
		else {
			slim_library_path += ":" + slim_library_location;
		}
	}
	slim_configuration_values["slim_library_path"] = slim_library_path;
	for(auto [key,value] : slim_configuration_values) {
		memory_mapper::write("slim_runtime_environmental_variables", key, value);
		auto check_value = memory_mapper::read_string("slim_runtime_environmental_variables", key);
#ifdef ENABLE_LOGGING
		log::debug(log::Message(__func__,"slim_runtime_environmental_variable => " + key + ":" + check_value,__FILE__,__LINE__));
#endif
	}
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"script arguments => " + script_arguments,__FILE__,__LINE__));
#endif
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
	return v8_configuration_values;
}
