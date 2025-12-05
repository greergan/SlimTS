#include <cstdlib>
#include <string>
#include <slim/common/network/address/set.h>
#include <slim/common/utilities.h>
bool slim::common::network::address::AddressSet::empty() {
	return (!port && address.empty()) ? true : false;
}
slim::common::network::address::AddressSet slim::common::network::address::string_to_address_set(std::string& address_string) {
	AddressSet address_set;
	if(!address_string.empty()) {
		int delimiter_position = address_string.find(":");
		if(delimiter_position) {
			address_set.address = address_string.substr(0, delimiter_position);
		}
		if(address_string.length() > delimiter_position) {
			auto port = slim::common::utilities::get_int_value(address_string.substr(delimiter_position + 1));
			if(port) {
				address_set.port = port;
			}
		}
	}
	return std::move(address_set);
}