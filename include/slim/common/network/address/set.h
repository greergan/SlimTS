#ifndef __SLIM__COMMON__NETWORK__ADDRESS__SET__H
#define __SLIM__COMMON__NETWORK__ADDRESS__SET__H
#include <string>
namespace slim::common::network::address {
	struct AddressSet {
		int port;
		std::string address;
		bool empty(void);
	};
	AddressSet string_to_address_set(std::string& address_string);
}
#endif