#include "config.h"
#include <filesystem>
#include <format>
#include <print>
#include <regex>
#include <system_error>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <v8.h>
#include <slim/command_line_handler.h>
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/memory/mapper.h>
#include <slim/path.h>

namespace slim::command_line {
namespace {
using namespace slim::common;

std::string script_args;
bool found_module_source_file = false;
std::vector<std::string> v8_config;
std::unordered_map<std::string, std::string> slim_config;
std::unordered_set<std::string> allowed_ext{"",".mjs",".ts"};
std::unordered_set<std::string> cdm_switches {"-d","-h","--help","-v","--version","-w"};
std::unordered_map<std::string, bool> arg_expects_values {};

std::string_view default_usage = "usage: slimts [options] <script>";

void help() {
    std::println("{}", default_usage);
    std::println("\t-d: detach server as daemon");
    std::println("\t-h, --help: display help");
    std::println("\t-v, --version: display version information");
    std::println("\t-w: reload script or types files on change");
    exit(1);
}

bool find_script(std::string_view s) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__,__LINE__});
    log::trace({__func__, std::format("checking for script name => {}", s), __FILE__,__LINE__});
#endif
    std::string script_name;
	if(s.starts_with("http://") || s.starts_with("https://")) {
		script_name = s;
		script_args += script_name + ",";
		slim_config["script_name"] = script_name;
		found_module_source_file = true;
	}
	else {
		auto script = std::regex_replace(s.data(), std::regex("/./|//"), "/");
		auto script_path = std::filesystem::absolute(script);

		if(allowed_ext.contains(script_path.extension().string()) && std::filesystem::exists(script_path)) {
			script_name = script_path.string();
			script_args += script_name + ",";
			slim_config["script_name"] = script_name;
			found_module_source_file = true;
		}
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("{} is script => {}", s, found_module_source_file),__FILE__,__LINE__});
	log::trace({__func__, "ends", __FILE__,__LINE__});
#endif
	return found_module_source_file;
}

bool is_slim_argument(std::string& argument) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__,__LINE__});
#endif
	auto answer = cdm_switches.contains(argument);
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("{} is_slim_argument => {}", argument, answer), __FILE__,__LINE__});
	log::trace({__func__, "ends", __FILE__,__LINE__});
#endif
	return answer;
}

bool is_arg_expects(std::string& argument) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__,__LINE__});
#endif
	auto answer = arg_expects_values.find(argument) != arg_expects_values.end() && arg_expects_values.find(argument)->second;
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("{} expects value => {}", argument, answer), __FILE__,__LINE__});
	log::trace({__func__, "ends", __FILE__,__LINE__});
#endif
	return answer;
}

void usage() {
    std::println("{}", default_usage);
	exit(1);
}

void version() {
        std::println("slimts: {}", VERSION);
        std::println("libv8:  {}", v8::V8::GetVersion());
        exit(0);
}
} // namespace
} // namespace slim::command_line

std::vector<std::string> slim::command_line::parse(int argc, char *argv[]) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
	slim_config["slim_executable"] = slim::path::getExecutablePath();
	script_args = slim_config["slim_executable"] + ",";

	for(int index = 1; index < argc; index++) {
		auto argument = std::string(argv[index]);
		if(found_module_source_file) {
#ifdef ENABLE_LOGGING
			log::debug({__func__, std::format("script argument => {}", argument), __FILE__,__LINE__});
#endif
			script_args += argument + ",";
		}
		else if(is_slim_argument(argument)) {
			if(argument == "-d") {
			memory_mapper::write("slim_runtime_environmental_variables", "daemon", true);
#ifdef ENABLE_LOGGING
			log::debug({__func__, "daemon => true", __FILE__,__LINE__});
#endif
			}
			else if(argument == "-h" || argument == "--help") {
                help();
			}
			else if(argument == "-v" || argument == "--version") {
				version();
			}
			else if(argument == "-w") {
				memory_mapper::write("slim_runtime_environmental_variables", "watching_files", true);
#ifdef ENABLE_LOGGING
				log::debug({__func__, "watching_files => true", __FILE__,__LINE__});
#endif
			}
			else if(is_arg_expects(argument)) {
				slim_config[argument] = argv[++index];
			}
		}
		else {
			if(!find_script(argument)) {
				auto argument_string = std::string(argv[index]);
#ifdef ENABLE_LOGGING
				log::debug({__func__, std::format("possible v8 argument found => {}", argument_string), __FILE__,__LINE__});
#endif
				v8_config.push_back(argument_string);
			}
		}
	}

	if(!found_module_source_file) {
	    if(!find_script("src/index.ts") && !find_script("src/index.mjs")) usage();
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, "command line has been parsed", __FILE__,__LINE__});
#endif
	if(script_args.ends_with(",")) {
		script_args.pop_back();
	}
	memory_mapper::write("configurations", "script.argv", script_args);
#ifdef ENABLE_LOGGING
	log::debug({__func__, "script arguments have been written", __FILE__,__LINE__});
#endif
	for(auto [key,value] : slim_config) {
		memory_mapper::write("slim_runtime_environmental_variables", key, value);
		auto check_value = memory_mapper::read_string("slim_runtime_environmental_variables", key);
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("slim_runtime_environmental_variable => {}:{}", key, check_value), __FILE__,__LINE__});
#endif
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("script arguments => {}", script_args), __FILE__,__LINE__});
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return v8_config;
}
