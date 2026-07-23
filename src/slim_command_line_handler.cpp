#include <filesystem>
#include <regex>
#include <system_error>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <slim/command_line_handler.h>
#include <slim/common/log.h>
#include <slim/common/memory/mapper.h>
#include <slim/path.h>

#include <iostream>

namespace slim::command_line {
	using namespace slim::common;

	std::string script_arguments;
	std::unordered_map<std::string, std::string> slim_configuration_values;
	std::vector<std::string> v8_configuration_values;
	std::vector<std::string> library_paths;
	std::unordered_set<std::string> allowed_file_extensions{"",".js",".mjs",".ts"};
	std::unordered_set<std::string> slim_configurations {"--lib", "--use-cache","--cache-dir","--create-configurations","-d","-v","-vv"};
	std::unordered_map<std::string, bool> slim_command_line_argument_expects_value {
		{"--lib",true},{"--cache-dir",true}
	};
	std::unordered_map<std::string, std::string> typescript_configurations
		{{"--print-typescript-all","false"},{"--print-typescript-debug","false"},
		{"--print-typescript-info","false"},{"--print-typescript-log","false"},{"--print-typescript-trace","false"},
		{"--print-typescript-warn","false"},{"--print-typescript-configuration","false"},
		{"--typescript-project",""}};
	std::unordered_map<std::string, bool> typescript_command_line_argument_expects_value {
		{"--typescript-project",true}
	};
	auto is_script = [](std::string& argument)->std::string {
		log::trace(log::Message("slim::command_line::is_script()","begins => " + argument, __FILE__,__LINE__));
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
		log::debug(log::Message("slim::command_line::is_script()",argument + " is_script => " + answer_message_string, __FILE__,__LINE__));
		log::trace(log::Message("slim::command_line::is_script()","ends => " + argument, __FILE__,__LINE__));
		return script_name_string;
	};
	auto is_slim_argument = [](std::string& argument)->bool {
		log::trace(log::Message("slim::command_line::is_slim_argument()","begins => " + argument, __FILE__,__LINE__));
		auto answer = slim_configurations.contains(argument);
		auto answer_message_string = + answer ? "true" : "false";
		log::debug(log::Message("slim::command_line::is_slim_argument()",argument + " is_slim_argument => " + answer_message_string, __FILE__,__LINE__));
		log::trace(log::Message("slim::command_line::is_slim_argument()","ends => " + argument, __FILE__,__LINE__));
		return answer;
	};
	auto slim_argument_expects_value = [](std::string& argument)->bool {
		log::trace(log::Message("slim::command_line::typescript_argument_expects_value()","begins => " + argument, __FILE__,__LINE__));
		auto answer = slim_command_line_argument_expects_value.find(argument) != slim_command_line_argument_expects_value.end()
			&& slim_command_line_argument_expects_value.find(argument)->second;
		auto answer_message_string = + answer ? "true" : "false";
		log::debug(log::Message("slim::command_line::typescript_argument_expects_value()",argument + " expects value => " + answer_message_string, __FILE__,__LINE__));
		log::trace(log::Message("slim::command_line::typescript_argument_expects_value()","ends => " + argument, __FILE__,__LINE__));
		return answer;
	};
	auto is_typescript_argument = [](std::string& argument)->bool {
		log::trace(log::Message("slim::command_line::is_typescript_argument()","begins => " + argument, __FILE__,__LINE__));
		auto answer = typescript_configurations.contains(argument);
		auto answer_message_string = + answer ? "true" : "false";
		log::debug(log::Message("slim::command_line::is_typescript_argument()",argument + " is_typescript_argument => " + answer_message_string, __FILE__,__LINE__));
		log::trace(log::Message("slim::command_line::is_typescript_argument()","ends => " + argument, __FILE__,__LINE__));
		return answer;
	};
	auto typescript_argument_expects_value = [](std::string& argument)->bool {
		log::trace(log::Message("slim::command_line::typescript_argument_expects_value()","begins => " + argument, __FILE__,__LINE__));
		auto answer = typescript_command_line_argument_expects_value.find(argument) != typescript_command_line_argument_expects_value.end()
			&& typescript_command_line_argument_expects_value.find(argument)->second;
		auto answer_message_string = + answer ? "true" : "false";
		log::debug(log::Message("slim::command_line::typescript_argument_expects_value()",argument + " expects value => " + answer_message_string, __FILE__,__LINE__));
		log::trace(log::Message("slim::command_line::typescript_argument_expects_value()","ends => " + argument, __FILE__,__LINE__));
		return answer;
	};
}
std::vector<std::string> slim::command_line::parse(int argc, char *argv[]) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	slim_configuration_values["slim_executable"] = slim::path::getExecutablePath();
	script_arguments = slim_configuration_values["slim_executable"] + ",";
	bool found_module_source_file = false;
	try {
		for(int index = 1; index < argc; index++) {
			auto argument = std::string(argv[index]);
			if(found_module_source_file) {
				log::debug(log::Message(__func__, "script argument => " + argument,__FILE__,__LINE__));
				script_arguments += argument + ",";
			}
			else if(is_typescript_argument(argument)) {
				log::debug(log::Message(__func__, "typescript argument => " + argument,__FILE__,__LINE__));
				if(argument.starts_with("--print")) {
					typescript_configurations[argument] = "true";
				}
				else if(typescript_argument_expects_value(argument)) {
					typescript_configurations[argument] = std::string(argv[++index]);
				}
			}
			else if(is_slim_argument(argument)) {
				log::debug(log::Message(__func__, "slim argument => " + argument,__FILE__,__LINE__));
				if(argument == "-d") {
					memory_mapper::write("slim_runtime_environmental_variables", "daemon", true);
					log::debug(log::Message(__func__,"daemon => true",__FILE__,__LINE__));
				}
				else if(argument == "--use-cache") {
					slim_configuration_values["use_cache"] = "true";
					log::debug(log::Message(__func__,"use_cache => true",__FILE__,__LINE__));
				}
				else if(argument == "--cache-dir") {
					auto cache_directory_argument = std::filesystem::path(argv[++index]);
					if(!std::filesystem::exists(cache_directory_argument)) {
						std::error_code error_code;
						if(std::filesystem::create_directories(cache_directory_argument, error_code)) {
							log::debug(log::Message(__func__,"cache_directory created => " + cache_directory_argument.string(),__FILE__,__LINE__));
						}
						else {
							const std::string error_string = "could not create cache_directory => " + cache_directory_argument.string() + " => " + error_code.message();
							log::error(log::Message(__func__,error_string,__FILE__,__LINE__));
							throw "slim::command_line::parse()|" + error_string;
						}
					}
					slim_configuration_values["cache_directory"] = std::filesystem::canonical(cache_directory_argument).string();
					log::debug(log::Message(__func__,"cache_directory => " + slim_configuration_values["cache_directory"],__FILE__,__LINE__));
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
					log::debug(log::Message(__func__, "script file name found => " + argument,__FILE__,__LINE__));
					found_module_source_file = true;
				}
				else {
					auto argument_string = std::string(argv[index]);
					log::debug(log::Message(__func__, "possible v8 argument found => " + argument_string,__FILE__,__LINE__));
					v8_configuration_values.push_back(argument_string);
				}
			}
		}
	}
	catch(const std::bad_alloc& bad_alloc_error) {
		const std::string error_message_string = "push_back failed => " + std::string(bad_alloc_error.what());
		log::error(log::Message(__func__, error_message_string,__FILE__, __LINE__));
		throw error_message_string;
	}
	log::debug(log::Message(__func__, "command line has been parsed",__FILE__,__LINE__));
	std::string typescript_configuration_string("{");
	for(auto [key,value] : typescript_configurations) {
		std::string new_key;
		std::string new_value;
		if(key.starts_with("--")) {
			new_key = key.substr(2);
		}
		new_key = "\"" + new_key + "\":";
		if(value == "true" || value == "false") {
			new_value = value + ",";
		}
		else {
			new_value = "\"" + value + "\",";
		}
		typescript_configuration_string += new_key + new_value;
	}
	if(typescript_configuration_string.ends_with(",")) {
		typescript_configuration_string.pop_back();
	}
	typescript_configuration_string += "}";
	log::debug(log::Message(__func__, "writting typescript configuration",__FILE__,__LINE__));
	memory_mapper::write("configurations", "typescript", typescript_configuration_string);
	log::debug(log::Message(__func__, "typescript configuration has been written",__FILE__,__LINE__));
	if(script_arguments.ends_with(",")) {
		script_arguments.pop_back();
	}
	memory_mapper::write("configurations", "script.argv", script_arguments);
	log::debug(log::Message(__func__, "script arguments have been written",__FILE__,__LINE__));
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
		log::debug(log::Message(__func__,"slim_runtime_environmental_variable => " + key + ":" + check_value,__FILE__,__LINE__));
	}
	log::debug(log::Message(__func__,"typescript arguments => " + typescript_configuration_string,__FILE__,__LINE__));
	log::debug(log::Message(__func__,"script arguments => " + script_arguments,__FILE__,__LINE__));
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
	return v8_configuration_values;
}
