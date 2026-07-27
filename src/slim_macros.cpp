#include <cstring>
#include <regex>
#include <string>
#include <sstream>
#include <slim/common/log.h>
#include <slim/macros.h>
#include <iostream>
namespace slim::macros {
	using namespace slim::common;
}
static void replace_token(const char* token, std::string& line_of_text, const std::string& replace_with_string, const bool is_text=false) {
	int file_name_token_position = 0;
	int quotes_space = is_text ? 2 : 0;
	while((file_name_token_position = line_of_text.find(token, file_name_token_position)) != std::string::npos) {
		if(is_text) {
			line_of_text.replace(file_name_token_position, strlen(token), "\"" + replace_with_string + "\"");
		}
		else {
			line_of_text.replace(file_name_token_position, strlen(token), replace_with_string);
		}
		file_name_token_position += replace_with_string.length() + quotes_space;
		//slim::common::log::debug(slim::common::log::Message("replace_token()", line_of_text.c_str(),__FILE__, __LINE__));
	}
}
std::unique_ptr<std::string> slim::macros::apply(std::shared_ptr<std::string> _content_pointer, const std::string& _specifier_string) {
	log::trace(log::Message("slim::macros::apply()","begins => " + _specifier_string,__FILE__, __LINE__));
	std::stringstream content_stream{std::string(*_content_pointer)};
	auto macro_applied_string_stream = apply(std::make_unique<std::stringstream>(std::move(content_stream)), _specifier_string);
	log::trace(log::Message("slim::macros::apply()","ends => " + _specifier_string,__FILE__, __LINE__));
	return std::make_unique<std::string>(std::move(macro_applied_string_stream->str()));
}
std::unique_ptr<std::stringstream> slim::macros::apply(std::unique_ptr<std::stringstream> _input_stringstream_pointer, const std::string& _absolute_file_path_string) {
	log::trace(log::Message("slim::macros::apply()","begins",__FILE__, __LINE__));
	int last_file_directory_separator = _absolute_file_path_string.find_last_of("/");
	std::string directory_name_string = _absolute_file_path_string.substr(0, last_file_directory_separator);
	std::string file_name_string = _absolute_file_path_string.substr(last_file_directory_separator + 1);
	std::stringstream output_string_stream;
	std::string line_of_text;
	int line_number = 0;
	while(std::getline(*_input_stringstream_pointer, line_of_text)) {
		//slim::common::log::debug(slim::common::log::Message("slim::macros::apply()", line_of_text.c_str(),__FILE__, __LINE__));
		line_number++;
		replace_token("__dirname", line_of_text, directory_name_string, true);
		replace_token("__filename", line_of_text, file_name_string, true);
		replace_token("__line_number", line_of_text, std::to_string(line_number), false);
		output_string_stream << line_of_text << std::endl;
	}
/* 	std::regex require_pattern("\\s*(const|let|var)\\s+(\\{*[,_[:alnum:][:s:]]+\\}*)\\s*=\\s*require\\s*\\(\\s*(\"|\')(\\w+)(\"|\')\\s*\\)");
	std::string import_statement = "\nimport $2 from '$4'";
	std::string return_string = std::regex_replace(output_string_stream.str(), require_pattern, import_statement); */
	log::trace(log::Message("slim::macros::apply()","ends",__FILE__, __LINE__));
	return std::make_unique<std::stringstream>(std::move(output_string_stream));
}