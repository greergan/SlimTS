#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/utilities.h>
#include <slim/SlimValue.hpp>

namespace slim::utilities {
	using namespace slim::common;
	slim::SlimValue can_parse(std::string_view _string);
	std::string_view control_character_to_string(const int _character);
	slim::SlimValue is_valid_scheme(const char* _string, const size_t _length, int& _start, int& _end);
	bool is_valid_ip(const char* _string, size_t _length);
	slim::SlimValue is_valid_host(const char* _string, size_t _length, int& end);
	bool is_valid_path(const char* _string, size_t _length);
}

std::string_view slim::utilities::control_character_to_string(const int _character) {
	std::string label;

	switch(_character) {
		case '\0': label = "Null"; break;
		case '\x01': label = "Start of Heading"; break;
		case '\x02': label = "Start of Text"; break;
		case '\x03': label = "End of Text"; break;
		case '\x04': label = "End of Transmission"; break;
		case '\x05': label = "Enquiry"; break;
		case '\x06': label = "Acknowledge"; break;
		case '\a': label = "Bell"; break;
		case '\b': label = "Backspace"; break;
		case '\t': label = "Horizontal Tab"; break;
		case '\n': label = "Line Feed"; break;
		case '\v': label = "Vertical Tab"; break;
		case '\f': label = "Form Feed"; break;
		case '\r': label = "Carriage Return"; break;
		case '\x0E': label = "Shift Out"; break;
		case '\x0F': label = "Shift In"; break;
		case '\x10': label = "Data Link Escape"; break;
		case '\x11': label = "Device Control 1"; break;
		case '\x12': label = "Device Control 2"; break;
		case '\x13': label = "Device Control 3"; break;
		case '\x14': label = "Device Control 4"; break;
		case '\x15': label = "Negative Acknowledge"; break;
		case '\x16': label = "Synchronous Idle"; break;
		case '\x17': label = "End of Transmission Block"; break;
		case '\x18': label = "Cancel"; break;
		case '\x19': label = "End of Medium"; break;
		case '\x1A': label = "Substitute"; break;
		case '\x1B': label = "Escape"; break;
		case '\x1C': label = "File Separator"; break;
		case '\x1D': label = "Group Separator"; break;
		case '\x1E': label = "Record Separator"; break;
		case '\x1F': label = "Unit Separator"; break;
		case '\x7F': label = "Delete"; break;
		default: label = "Unknown Control Character"; break;
	}

	return label;
}

slim::SlimValue slim::utilities::can_parse(std::string_view _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("std::string => \"{}\" => size_t => {}", _string, _string.size()), __FILE__,__LINE__));

	slim::SlimValue return_value = true;
	size_t string_length = _string.size();
	auto error_map = return_value.get_multi_map("errors");
	if(!string_length == 0) {
		return_value = false;
		error_map.set("url", "string is empty");
	}
	else {
		
		int protocol_start_position = -1;
		int protocal_end_position = -1;
		bool has_control_characters = false;
		bool host_found;
		bool protocol_found = false;
		bool port_found = false;
		bool space_in_host = false;
		size_t string_position = 0;
		for(; string_position < string_length; ++string_position) {
			char character = _string[string_position];
			if(std::iscntrl(character)) {
				error_map.set("invalid_character", control_character_to_string(character));
			}
			else if(character == ':') {
				if(!protocol_found) {
					if(_string[string_position + 1] == '/' && _string[string_position + 2] == '/') {
						protocol_found = true;
						protocol_start_position = string_position;
						protocal_end_position = string_position + 2;
						string_position += 2;
					}
				}
				else {
					return_value = false;
					return_value.set_error("protocal delimiter not found => \"://\"");
				}
			}
			else if(character == ' ') {
				if(!protocol_found) {
					space_in_host = true;
				}
			}
		}
	}

	if(error_map.size() > 0) {
		return_value.set_error("URL is unparsable");
	}
	log::debug(log::Message(__func__, return_value.get_error().message_or("successful"), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return return_value;
}

slim::SlimValue slim::utilities::is_valid_host(const char* _string, size_t _length, int& _end) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("const char* => \"{}\" => size_t => {}", _string, _length), __FILE__, __LINE__));

	slim::SlimValue return_value;

	if(!_string) {
		return_value = false;
		return_value.set_error("string is null");
	}
	else if(_length == 0) {
		return_value = false;
		return_value.set_error("length is 0");
	}
	else if(_length > 253) {
		return_value = false;
		return_value.set_error("length exceeds 253");
	}

	if(return_value.has_error()) {
		log::debug(log::Message(__func__, return_value.get_error().message_or("failed"), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return return_value;
	}

	int label_length = 0;
	int label_count = 0;

	int ipv4_digit_count = 0;
	int ipv4_dot_count = 0;
	int ipv4_value = 0;
	bool ipv4_candidate = true;

	for(size_t character_index = 0; character_index <= _length; character_index++) {
		char current_character = (character_index < _length) ? _string[character_index] : '.';

		unsigned char unsigned_character = static_cast<unsigned char>(current_character);

		bool is_digit = (unsigned_character ^ '0') <= 9;
		bool is_letter = ((unsigned_character | 32) - 'a' <= 25);
		bool is_hyphen = (current_character == '-');
		bool is_dot = (current_character == '.');

		if(is_digit) {
			ipv4_value = ipv4_value * 10 + (current_character - '0');
			ipv4_digit_count++;
			label_length++;
			continue;
		}

		if(is_dot) {
			ipv4_dot_count++;

			if(ipv4_digit_count == 0 || ipv4_value > 255) {
				ipv4_candidate = false;
			}

			ipv4_value = 0;
			ipv4_digit_count = 0;

			if(label_length == 0 || label_length > 63) {
				return_value = false;
				return_value.set_error("invalid label length");

				log::debug(log::Message(__func__, return_value.get_error().message_or("failed"), __FILE__, __LINE__));
				log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
				return return_value;
			}

			label_length = 0;
			label_count++;
			_end = static_cast<int>(character_index);
			continue;
		}

		if(is_letter || is_hyphen) {
			ipv4_candidate = false;
			label_length++;
			continue;
		}

		return_value = false;
		return_value.set_error("invalid character in host");

		log::debug(log::Message(__func__, return_value.get_error().message_or("failed"), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return return_value;
	}

	if(ipv4_candidate && ipv4_dot_count == 3) {
		return_value = true;

		log::debug(log::Message(__func__, "valid IPv4 host detected", __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return return_value;
	}

	if(label_count == 0 || label_length == 0) {
		return_value = false;
		return_value.set_error("invalid host structure");

		log::debug(log::Message(__func__, return_value.get_error().message_or("failed"), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return return_value;
	}

	_end = static_cast<int>(_length);

	return_value = true;

	log::debug(log::Message(__func__, "valid hostname detected", __FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));

	return return_value;
}



slim::SlimValue slim::utilities::is_valid_scheme(const char* _string, const size_t _length, int& _start, int& _end) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("const char* => \"{}\" => size_t => {}", _string, _length), __FILE__,__LINE__));
	slim::SlimValue return_value;
	if(!_string) {
		return_value = false;
		return_value.set_error("string is null");
	}
	else if(_length < 2) {
		return_value = false;
		return_value.set_error("string is too short");
	}

	if(return_value.has_error()) {
		log::debug(log::Message(__func__, return_value.get_error().message_or("failed"), __FILE__,__LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
		return return_value;
	}

	size_t character_index = 0;
	while(character_index < _length && _string[character_index] == ' ') {
		++character_index;
	}
	_start = character_index;

	for(; character_index < _length; character_index++) {
		char current_character = _string[character_index];

		if(current_character == ' ') {
			return_value = false;
			return_value.set_error("space not allowed in scheme");
			break;
		}
		else if(current_character == ':') {
			if(character_index > 0) {
				_end = character_index;
				return_value = true;
			} else {
				return_value = false;
				return_value.set_error("empty scheme");
			}
			break;
		}
		else if(!std::isalpha(current_character)) {
			return_value = false;
			return_value.set_error("invalid character in scheme");
			break;
		}
	}

	log::debug(log::Message(__func__, return_value.get_error().message_or("success"), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return return_value;
}


namespace slim::common::http {
	using namespace slim::common;
	static const std::regex protocol_pattern(R"(^(https?|ftp)://([A-Za-z0-9-]+\.)+[A-Za-z]{2,}(:\d+)?(/[^ \t\r\n]*)?$)",std::regex::icase);
	static const std::regex url_pattern(R"(^(?:(https?|file|ftp|wss?)://)?(?:([^/:?#]+)(?::(\d+))?)?([^?#]*)(?:\?([^#]*))?(?:#(.*))?$)",std::regex::icase);

}

slim::common::http::URL::URL() {}
slim::common::http::URL::URL(const std::string& _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	parse(_string);
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
}

const slim::SlimValue slim::common::http::URL::can_parse(std::string_view _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));

	auto result = slim::utilities::can_parse(_string);

	log::debug(log::Message(__func__, std::format("{} => returns => {} ", _string, result.to_string()), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return result;
}
const bool slim::common::http::URL::is_valid() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, "return =>" + utilities::to_string(__is_valid), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
    return __is_valid;
}
void slim::common::http::URL::parse(const std::string& _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));

    std::smatch match;
	if(std::regex_match(_string, match, url_pattern)) {
		__protocol = match[1];
		__hostname = match[2];
		__port     = match[3];
		__pathname = match[4];
		__search   = match[5];
		__hash     = match[6];

		if(__protocol == "") {
			__protocol = "https";
		}

		if(__port == "") {
			if(__protocol == "http" || __protocol == "ws") {
				__port = "80";
			}
			else if(__protocol == "https" || __protocol == "wss") {
				__port = "443";
			}
			else if(__protocol == "ftp") {
				__port = "21";
			}
		}

		if(__hostname != "") {
			__host = __port != "" ? std::format("{}:{}", __hostname.value(), __port.value()) : __hostname;
		}

		if(__pathname == "") {
			__pathname = "/";
		}

		if(__protocol == "file" && !__pathname.value().starts_with("/")) {
			__pathname = (std::filesystem::absolute(__pathname.value()).string());
			log::debug(log::Message(__func__,"set request absolute file path => " + __pathname.value(),__FILE__,__LINE__));
		}
		else if(__protocol.value().starts_with("http") && __pathname.value().length() > 1) {
			slim::common::utilities::replace_all(__pathname.value(), "..", "");
			slim::common::utilities::replace_all(__pathname.value(), "/.", "/");
			slim::common::utilities::replace_all(__pathname.value(), "./", "/");
			slim::common::utilities::replace_all(__pathname.value(), "//", "/");
		}

		// must run after required bits have been set
		if(__protocol != "" && __host.has_value() && __host != "" && __pathname != "") {
			__href = std::format("{}://{}{}", __protocol.value(), __host.value(), __pathname.value());
		}
		__url = _string;
		__is_valid = true;
		log::debug(log::Message(__func__, "URL parsed successfully", __FILE__,__LINE__));
    }
	else {
		__is_valid = false;
		log::debug(log::Message(__func__, "URL was not parsed successfully => " + _string, __FILE__,__LINE__));
	}
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
}

const std::optional<std::string> slim::common::http::URL::hash() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __hash.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __hash;
}
const std::optional<std::string> slim::common::http::URL::host() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => host => {}", __host.value_or("is not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __host;
}
const std::optional<std::string> slim::common::http::URL::hostname() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __hostname.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __hostname;
}
const std::optional<std::string> slim::common::http::URL::href() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __host.value_or("is not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __href;
}
const std::optional<std::string> slim::common::http::URL::origin() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __origin.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __origin;
}
const std::optional<std::string> slim::common::http::URL::password() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __password.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __password;
}
const std::optional<std::string> slim::common::http::URL::pathname() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __pathname.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __pathname;
}
const std::optional<std::string> slim::common::http::URL::port() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __port.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __port;
}
const std::optional<std::string> slim::common::http::URL::protocol() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __protocol.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __protocol.value();
}
const std::optional<std::string> slim::common::http::URL::search() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::error(log::Message(__func__, "full parsing not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, "full parsing not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __search.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __search;
}
const std::optional<std::string> slim::common::http::URL::searchParams() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::error(log::Message(__func__, "not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, "not implemented =>", __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return "";
}
const std::optional<std::string> slim::common::http::URL::username() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __username.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __username;
}

const std::optional<std::string> slim::common::http::URL::url() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __url.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __url;
}