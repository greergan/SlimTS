#ifndef __SLIM__COMMON__NETWORK__CLIENT__TCP__H
#define __SLIM__COMMON__NETWORK__CLIENT__TCP__H
#include <memory>
#include <netinet/in.h>
#include <netdb.h>
#include <slim/common/network/address/set.h>
#include <slim/common/web_file.h>
namespace slim::common::network::client::tcp {
	constexpr int BUFFER_SIZE = 1024;
	struct Connection {
		Connection(slim::common::WebFile* web_file_pointer);
		private:
			int socket_handle;
			struct sockaddr_in server_address;
			struct addrinfo hints, *addrinfo_pointer;
	};
}
#endif