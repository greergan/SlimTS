#include <algorithm>
#include <charconv>
#include <optional>
//#include <cctype>
#include <string>
#include <string_view>
#include <slim/common/network/address_set.h>
#include <slim/common/utilities.h>
[[nodiscard]]
const std::string slim::common::utilities::to_string(const bool _value) {
	return _value ? "true" : "false";
}
[[nodiscard]]
bool slim::common::utilities::get_bool_value(std::string_view _string) {
/* 	if(!_string.empty()) {
		std::string string_value(_string);
		std::transform(string_value.begin(), string_value.end(), string_value.begin(), ::tolower);
		if(string_value == "true") {
			return true;
		}
        try {
            return std::stoi(std::string(string_value));
        }
        catch (std::invalid_argument const& ex) {
            return false;
        }
        catch (std::out_of_range const& ex) {
			return false;
        }
	} */
	return false;
}

void slim::common::utilities::replace_all(std::string& _string, const std::string& _original, const std::string& _replacement) {
    if(_string.empty() || _original.empty() || _replacement.empty()) {
		return;
	}

    size_t start_pos = 0;
    while((start_pos = _string.find(_original, start_pos)) != std::string::npos) {
        _string.replace(start_pos, _original.length(), _replacement);
        start_pos += _replacement.length();
    }
}

[[nodiscard]]
std::optional<int> slim::common::utilities::to_int(std::string_view _string) {
	int result = {};
	const auto respose = std::from_chars(_string.begin(), _string.end(), result);
	return result;
}

[[nodiscard]]
std::string slim::common::utilities::to_lower(std::string_view _string) {
	std::string return_string;
	std::transform(_string.begin(), _string.end(), return_string.begin(), [](unsigned char c){ return std::tolower(c); });
	return return_string;
}