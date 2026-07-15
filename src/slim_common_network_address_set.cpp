#include <string_view>
#include <slim/common/network/address_set.h>
#include <slim/common/utilities.h>
slim::common::network::AddressSet::AddressSet() {}
slim::common::network::AddressSet::AddressSet(std::string_view _string) {
	if(!_string.empty()) {
		int delimiter_position = _string.find(":");
		if(delimiter_position > 0) {
			_address = _string.substr(0, delimiter_position);
			if(_string.length() > delimiter_position) {
				const auto has_port = slim::common::utilities::to_int(_string.substr(delimiter_position + 1));
				if(has_port.has_value()) {
					_port = has_port.value();
				}
			}
		}
		else {
			_address = _string;
		}
	}
}
const std::string& slim::common::network::AddressSet::address() const {
	return _address.value();
}
const bool slim::common::network::AddressSet::empty() const {
	return (!_port.has_value() || !_address.has_value()) ? true : false;
}
const int slim::common::network::AddressSet::port() const {
	return _port.value();
}
